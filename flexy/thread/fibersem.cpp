#include "fibersem.h"
#include "flexy/schedule/scheduler.h"
#include "flexy/util/macro.h"

namespace flexy {

FiberSemaphore::~FiberSemaphore() {
    FLEXY_ASSERT(waiters_.empty());
}

bool FiberSemaphore::tryWait() {
    FLEXY_ASSERT(Scheduler::GetThis());
    {
        LOCK_GUARD(mutex_);
        if (concurrency_ > 0u) {
            --concurrency_;
            return true;
        } 
        return false;
    }
}

void FiberSemaphore::wait() {
    FLEXY_ASSERT(Scheduler::GetThis());
    {
        LOCK_GUARD(mutex_);
        if (concurrency_ > 0u) {
            --concurrency_;
            return;
        }
        waiters_.emplace(Scheduler::GetThis(), Fiber::GetThis());
    }
    Fiber::Yield();
}

void FiberSemaphore::post() {
    LOCK_GUARD(mutex_);
    if (!waiters_.empty()) {
        auto&& [scheduler, fiber] = std::move(waiters_.front());
        waiters_.pop();
        scheduler->async(std::move(fiber));
    } else {
        ++concurrency_;
    }
}

} // namespace flexy