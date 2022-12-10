#include "condition_variable.h"
#include "flexy/schedule/scheduler.h"
#include "flexy/util/macro.h"

namespace flexy::fiber {

void condition_variable::notify_all() noexcept {
    decltype(waiters_) waiters;
    {
        LOCK_GUARD(mutex_);
        waiters.swap(waiters_);
    }
    for (auto& [scheduler, fiber] : waiters) {
        scheduler->async(std::move(fiber));
    }
}

void condition_variable::notify_one() noexcept {
    std::pair<Scheduler*, Fiber::ptr> waiter;
    {
        LOCK_GUARD(mutex_);
        if (waiters_.empty()) {
            return;
        }
        waiter = std::move(waiters_.front());
        waiters_.pop_front();
    }
    waiter.first->async(std::move(waiter.second));
}

// 两段相同的代码 暂时没想到协程锁和条件表量有什么可以优化的地方
void condition_variable::wait(unique_lock<mutex>& __lock) noexcept {
    FLEXY_ASSERT(Scheduler::GetThis());
    FLEXY_ASSERT2(Fiber::GetFiberId() != 0, "Main Fiber cannot wait");
    {
        LOCK_GUARD(mutex_);
        waiters_.emplace_back(Scheduler::GetThis(), Fiber::GetThis());
    }
    // __lock.unlock();
    // Fiber::Yield();
    Fiber::GetThis()->yield_callback(
        [&__lock]() {  // 保证 yield 和 unlock的原子性
            __lock.unlock();
        });

    __lock.lock();
}

void condition_variable::wait(unique_lock<flexy::mutex>& __lock) noexcept {
    FLEXY_ASSERT(Scheduler::GetThis());
    FLEXY_ASSERT2(Fiber::GetFiberId() != 0, "Main Fiber cannot wait");
    {
        LOCK_GUARD(mutex_);
        waiters_.emplace_back(Scheduler::GetThis(), Fiber::GetThis());
    }
    // __lock.unlock();
    // Fiber::Yield();
    Fiber::GetThis()->yield_callback(
        [&__lock]() {  // 保证 yield 和 unlock的原子性
            __lock.unlock();
        });

    __lock.lock();
}

}  // namespace flexy::fiber