#include "flexy/fiber/mutex.h"
#include "flexy/schedule/scheduler.h"
#include "flexy/util/macro.h"

namespace flexy::fiber {

mutex::~mutex() { FLEXY_ASSERT(locked_ == false); }

void mutex::lock() {
    FLEXY_ASSERT(Scheduler::GetThis());
    FLEXY_ASSERT2(Fiber::GetFiberId() != 0, "Main Fiber cannot wait");
    {
        LOCK_GUARD(mutex_);
        if (locked_) {
            waiters_.emplace_back(Scheduler::GetThis(), Fiber::GetThis());
        } else {
            locked_ = true;
            return;
        }
    }
    Fiber::Yield();
}

void mutex::unlock() {
    std::pair<Scheduler*, Fiber::ptr> waiter;
    {
        LOCK_GUARD(mutex_);
        FLEXY_ASSERT(locked_);
        if (waiters_.empty()) {
            locked_ = false;
            return;
        } else {
            waiter = std::move(waiters_.front());
            waiters_.pop_front();
        }
    }
    waiter.first->async(std::move(waiter.second));
}

}  // namespace flexy::fiber