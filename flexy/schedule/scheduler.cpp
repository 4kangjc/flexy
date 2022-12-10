#include "scheduler.h"
#include "flexy/util/macro.h"
#include "flexy/net/hook.h"

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");
static thread_local Scheduler* t_scheduler = nullptr;  // 线程所属的协程调度器

Scheduler::Scheduler(size_t threads, bool use_caller, std::string_view name) : name_(name) {
    FLEXY_ASSERT(threads > 0);

    if (use_caller) {
        Fiber::GetThis();
        --threads;

        FLEXY_ASSERT(t_scheduler == nullptr);
        t_scheduler = this;

        Thread::SetName(name);

        rootThreadId_ = GetThreadId();
        threadIds_.push_back(rootThreadId_);
    } else {
        rootThreadId_ = -1;
    }
    threadCount_ = threads;

    idle_ = [this]() {
        FLEXY_LOG_INFO(g_logger) << "idle";
        while (!stopping()) {
            Fiber::Yield();
        }
    };

    tickle_ = []() { FLEXY_LOG_DEBUG(g_logger) << "tickle"; };
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::start() {
    LOCK_GUARD(mutex_);
    if (!stopping_) {
        return;
    }
    stopping_ = false;
    FLEXY_ASSERT(threads_.empty());
    threads_.reserve(threadCount_);
    for (size_t i = 0; i < threadCount_; ++i) {
        threads_.emplace_back(std::make_shared<Thread>(name_ + "_" + std::to_string(i), 
            &Scheduler::run, this));
        threadIds_.push_back(threads_[i]->getId());
    }
}

void Scheduler::stop() {
    if (threadCount_ == 0) {
        FLEXY_LOG_INFO(g_logger) << this << " stopped";
        stopping_ = true;
        if (stopping()) {
            return;
        }
    }

    if (rootThreadId_ != -1) {
        FLEXY_ASSERT(GetThis() == this);        // 只能由caller线程发起stop
    } else {
        FLEXY_ASSERT(GetThis() != this);
    }

    stopping_ = true;
    for (size_t i = 0; i < threadCount_; ++i) {
        tickle_();
    }

    if (rootThreadId_ != -1) {  // usercaller 调度协程执行 run()
        if (!stopping()) {
            run();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        LOCK_GUARD(mutex_);
        thrs.swap(threads_);
    }
    for (auto& thr : thrs) {
        thr->join();
    }
}


void Scheduler::run() {
    FLEXY_LOG_DEBUG(g_logger) << name_ << " run";
    set_hook_enable(true);                          // 启用hook
    setThis();
    if (GetThreadId() != rootThreadId_) {                   // 非主线程
        Fiber::GetThis();
    }
    auto idle_fiber(fiber_make_shared([this]() { idle_(); }));
    // auto idle_fiber(std::make_shared<Fiber>(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;
    
    Task tk;
    while (true) {
        // tk.reset();
        bool tickle_me = false;
        // get task from deque
        {
            LOCK_GUARD(mutex_);
            if (!tasks_.empty()) {
                tk = std::move(tasks_.front());
                tasks_.pop_front();
                if (!tk)   continue;            // nullptr task continue
                if (tk.fiber &&
                    FLEXY_UNLIKELY(tk.fiber->getState() == Fiber::EXEC)) {
                    tasks_.push_back(std::move(tk));
                    continue;
                }
                ++activeThreadCount_;
                tickle_me = true;
            }
        }

        if (tickle_me) {
            tickle_();
        }

        if (tk.fiber) {
            tk.fiber->resume();
            --activeThreadCount_;
            tk.reset();
        } else if (tk.cb) {
            if (cb_fiber) {
                cb_fiber->reset(std::move(tk.cb));
                // cb_fiber->reset(tk.cb);
            } else {
                // cb_fiber.reset(new Fiber(std::move(tk.cb)));
                cb_fiber = fiber_make_shared(std::move(tk.cb));
            }
            cb_fiber->resume();
            --activeThreadCount_;
            tk.reset();
            if (cb_fiber->state_ != Fiber::TERM) {
                cb_fiber.reset();
            }
        } else {            //  没有任务做，执行idle协程
            if (idle_fiber->getState() == Fiber::TERM) {
                FLEXY_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            ++idleThreadCount_;
            idle_fiber->resume();
            --idleThreadCount_;
        }
    }
}

Scheduler::~Scheduler() {
    FLEXY_ASSERT(stopping_);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

void Scheduler::tickle() {
    FLEXY_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    LOCK_GUARD(mutex_);
    return stopping_ && tasks_.empty() && activeThreadCount_ == 0;
}

void Scheduler::idle() {
    FLEXY_LOG_INFO(g_logger) << "idle";
    while (!stopping()) {
        Fiber::Yield();
    }
}

std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name = " << name_ << " size = " << threadCount_
       << " active_count = " << activeThreadCount_
       << " idle_count = " << idleThreadCount_ << " stopping = " << stopping_
       << " ]" << std::endl
       << "    ";
    for (size_t i = 0; i < threadIds_.size(); ++i) {
        if (i) {
            os << ", ";
        }
        os << threadIds_[i];
    }
    return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler* target) {
    caller_ = Scheduler::GetThis();
    if (target) {
        SwitchTo(target);
    }
}

SchedulerSwitcher::~SchedulerSwitcher() {
    if (caller_) {
        SwitchTo(caller_);
    }
}

void SchedulerSwitcher::SwitchTo(Scheduler* target) {
    FLEXY_ASSERT(Scheduler::GetThis() != nullptr);
    if (Scheduler::GetThis() == target) {
        return;
    }
    target->async(Fiber::GetThis());
    Fiber::Yield();
}

} // namespace flexy