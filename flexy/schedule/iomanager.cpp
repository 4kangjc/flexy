#include "iomanager.h"
#include "flexy/util/macro.h"
#include "flexy/util/config.h"

#include <sys/epoll.h>
#include <fcntl.h>

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");
static auto g_channel_init_size = Config::Lookup("channel.init.size", 64, "channel vector init size");
static auto g_channel_resize_times = Config::Lookup("channel.resize.times", 2.0f, "channel vector resize times");

IOManager::IOManager(size_t threads, bool use_caller, std::string_view name) 
            : Scheduler(threads, use_caller, name) {
    epfd_ = epoll_create(5);
    FLEXY_ASSERT(epfd_ > 0);

    int rt = pipe(tickleFds_);
    FLEXY_ASSERT(!rt);
    
    // epoll_event ev;
    // memset(&ev, 0, sizeof(ev));
    // ev.events = EPOLLET | EPOLLIN;
    // ev.data.fd = tickleFds_[0];

    rt = fcntl(tickleFds_[0], F_SETFL, O_NONBLOCK);
    FLEXY_ASSERT(!rt);

    // rt = epoll_ctl(epfd_, EPOLL_CTL_ADD, tickleFds_[0], &ev);
    // FLEXY_ASSERT(!rt);

    channelResize(g_channel_init_size->getValue() * 1);

    channels_[tickleFds_[0]]->enableRead();

    start();
}

IOManager::~IOManager() {
    stop();
    close(epfd_);
    close(tickleFds_[0]);
    close(tickleFds_[1]);

    for (size_t i = 0; i < channels_.size(); ++i) {
        delete channels_[i];
    }
}


bool IOManager::delEvent(int fd, Event event) {
    Channel* ch = nullptr;
    {
        LOCK_GUARD(mutex_);
        if ((int)channels_.size() <= fd) {
            return false;
        }
        ch = channels_[fd];
    }

    LOCK_GUARD(ch->mutex_);

    if (FLEXY_UNLIKELY(!(ch->getEvents() & event))) {
        return false;
    }

    bool res = ch->enableEvents(event, false);
    if (!res) {
        return false;
    }

    --pendingEventCount_;
    auto& event_ctx = ch->getContext(event);
    ch->resetContext(event_ctx);

    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    Channel* ch = nullptr;
    {
        LOCK_GUARD(mutex_);
        if ((int)channels_.size() <= fd) {
            return false;
        }
        ch = channels_[fd];
    }

    LOCK_GUARD(ch->mutex_);


    if (FLEXY_UNLIKELY(!(ch->getEvents() & event))) {
        return false;
    }

    ch->handleEvents(event);
    --pendingEventCount_;

    return true;
}

bool IOManager::cancelAll(int fd) {
    Channel* ch = nullptr;
    {
        LOCK_GUARD(mutex_);
        if ((int)channels_.size() <= fd) {
            return false;
        }
        ch = channels_[fd];
    }

    LOCK_GUARD(ch->mutex_);

    if (!ch->getEvents()) {
        return false;
    }

    int events = ch->getEvents();
    int cacelEvent = 0;
    
    if (events & Event::READ) {
        cacelEvent |= Event::READ;
    }
    if (events & Event::WRITE) {
        cacelEvent |= WRITE;
    }

    if (cacelEvent == 0) {
        return true;
    }

    int count = ch->handleEvents((Event)cacelEvent);

    pendingEventCount_ -= count;

    FLEXY_ASSERT(ch->getEvents() == 0);

    return true;
}

bool IOManager::onEvent(int fd, Event event, __task&& cb) {
    Channel* ch = nullptr;
    {
        LOCK_GUARD(mutex_);
        if ((int)channels_.size() <= fd) {
            channelResize(g_channel_resize_times->getValue() * fd);
        }
        ch = channels_[fd];
    }

    LOCK_GUARD(ch->mutex_);

    if (FLEXY_UNLIKELY(ch->getEvents() & event)) {
        FLEXY_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd << " event = " << event
            << " ch.event = " << ch->getEvents();
        FLEXY_ASSERT(!(ch->getEvents() & event));
    }

    bool res = ch->enableEvents(event);
    if (!res) {
        return false;
    }
    
    ++pendingEventCount_;
    
    auto& event_ctx = ch->getContext(event);
    FLEXY_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
    event_ctx.scheduler = Scheduler::GetThis();

    if (cb) {
        event_ctx.cb = std::move(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        FLEXY_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC, "state = " << event_ctx.fiber->getState());
    }

    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::channelResize(size_t size) {
    channels_.resize(size);
    for (size_t i = 0; i < channels_.size(); ++i) {
        if (!channels_[i]) {
            channels_[i] = new Channel(epfd_, i);
        }
    }
}

void IOManager::tickle() {
    if (!hasIdleThreads()) {
        return;
    }
    int rt = write(tickleFds_[1], "", 1);
    FLEXY_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull && pendingEventCount_ == 0 && Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {
    FLEXY_LOG_DEBUG(g_logger) << "idle";
    // epoll_event* evs = new 
    std::unique_ptr<epoll_event[]> events(new epoll_event[256]);
    while (true) {
        uint64_t next_timeout = 0;
        if (FLEXY_UNLIKELY(stopping(next_timeout))) {
            FLEXY_LOG_FMT_INFO(g_logger, "IOManager name = {} idle stopping exit", Scheduler::getName());
            break;
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            if (next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(epfd_, events.get(), 256, next_timeout);
            if (rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while(true);

        auto cbs = listExpiriedTimer();
        if (!cbs.empty()) {
            async(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for (int i = 0; i < rt; ++i) {
            epoll_event& ev = events[i];
            Channel* ch = (Channel*)ev.data.ptr;
            if (ch->fd() == tickleFds_[0]) {
                uint8_t dummy[256];
                while (read(tickleFds_[0], &dummy, sizeof(dummy)) > 0);
                continue;
            }
            LOCK_GUARD(ch->mutex_);
            if (ev.events & (EPOLLERR | EPOLLHUP)) {
                ev.events |= (EPOLLIN | EPOLLOUT) & ch->getEvents();
            }

            int real_event = Event::NONE;
            if (ev.events & EPOLLIN) {
                real_event |= Event::READ;
            }
            if (ev.events & EPOLLOUT) {
                real_event |= Event::WRITE;
            }

            if ((ch->getEvents() & real_event) == 0) {
                continue;
            }

            int count = ch->handleEvents((Event)real_event);
            pendingEventCount_ -= count;
        }

        Fiber::Yield();
    }
}

void IOManager::onTimerInsertedAtFront() {
    return tickle();
}

} // namespace flexy