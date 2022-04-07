#include "channel.h"
#include "flexy/util/macro.h"

#include <sys/epoll.h>

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

static std::ostream& operator<<(std::ostream& os, EPOLL_EVENTS events) {
    if (!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E) \
    if(events & E) { \
        if(!first) { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

int Channel::handleEvents(Event events) {
    enableEvents(events, false);
    int res = 0;
    if (events & Event::READ) {
        // handleRead();
        if (read_.cb) {
            read_.scheduler->async(std::move(read_.cb));
        } else {
            read_.scheduler->async(std::move(read_.fiber));
        }
        read_.scheduler = nullptr;
        ++res;
    }

    if (events & Event::WRITE) {
        // handleWrite();
        if (write_.cb) {
            write_.scheduler->async(std::move(write_.cb));
        } else {
            write_.scheduler->async(std::move(write_.fiber));
        }
        write_.scheduler = nullptr;
        ++res;
    }
    return res;
}

void Channel::handleRead() {
    if (read_.cb) {
        read_.scheduler->async(std::move(read_.cb));
    } else {
        read_.scheduler->async(std::move(read_.fiber));
    }
    read_.scheduler = nullptr;
    enableRead(false);
}

void Channel::handleWrite() {
    if (write_.cb) {
        write_.scheduler->async(std::move(write_.cb));
    } else {
        write_.scheduler->async(std::move(write_.fiber));
    }
    write_.scheduler = nullptr;
    enableWrite(false);
}

Channel::EventContext& Channel::getContext(Event event) {
    switch (event) {
        case Event::READ:
            return read_;
        case Event::WRITE:
            return write_;
        default:
            FLEXY_ASSERT2(false, "getContext failed");
    }
    throw std::invalid_argument("getContext invalid event");
}


Channel::Channel(int epoll_fd, int fd, Event evens) : epfd_(epoll_fd), fd_(fd), events_(evens) { }

bool Channel::enableRead(bool enable) {
    return enableEvents(Event::READ, enable);
}

bool Channel::enableWrite(bool enable) {
    return enableEvents(Event::WRITE, enable);
}

bool Channel::enableEvents(Event events, bool enable) {
    if (enable) {
        int op = events_ ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event ev;
        ev.events = EPOLLET | events_ | events;
        ev.data.ptr = this;
        int rt = epoll_ctl(epfd_, op, fd_, &ev);
        if (rt) {
            FLEXY_LOG_FMT_ERROR(g_logger, "epoll_ctl({}, {}, {}) : {} ({}) ({})", 
                    epfd_, op, fd_, rt, errno, strerror(errno));
            FLEXY_LOG_ERROR(g_logger) << "events = " << events << ", ev.events = " 
                << (EPOLL_EVENTS)ev.events << flexy::BacktraceToString(100, 2, "    ");
            return false;
        }
        events_ = (Event)(events_ | events);
    } else {
        int new_events = events_ & ~events;
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ev;
        ev.events = EPOLLET | new_events;
        ev.data.ptr = this;
        int rt = epoll_ctl(epfd_, op, fd_, &ev);
        if (rt) {
            FLEXY_LOG_FMT_ERROR(g_logger, "epoll_ctl({}, {}, {}) : {} ({}) ({})", 
                    epfd_, op, fd_, rt, errno, strerror(errno));
            FLEXY_LOG_ERROR(g_logger) << "events = " << events << ", ev.events = " 
                << (EPOLL_EVENTS)ev.events << flexy::BacktraceToString(100, 2, "    ");
            return false;
        }
        events_ = (Event)new_events;
    }
    return true;
}

void Channel::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber = nullptr;
    ctx.cb = nullptr;
}


} // namespace flexy