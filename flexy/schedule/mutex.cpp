#include "mutex.h"
#include "scheduler.h"
#include "flexy/util/macro.h"

namespace flexy::fiber {

using namespace flexy;

mutex::~mutex() {
    FLEXY_ASSERT(locked_ == false);
}

void mutex::lock() {
    FLEXY_ASSERT(Scheduler::GetThis());
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


} // namespace flexy fiber