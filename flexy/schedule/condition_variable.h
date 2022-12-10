#pragma once

#include "flexy/util/noncopyable.h"
// #include "flexy/thread/mutex.h"
#include <condition_variable>
#include <deque>
#include "mutex.h"

namespace flexy {

class Scheduler;

}

namespace flexy::fiber {

class condition_variable : noncopyable {
public:
    condition_variable() noexcept = default;
    ~condition_variable() noexcept = default;

    void notify_one() noexcept;
    void notify_all() noexcept;
    // wait std::mutex
    void wait(unique_lock<flexy::mutex>& __lock) noexcept;
    // wait fiber::mutex
    void wait(unique_lock<mutex>& __lock) noexcept;

    template <typename _Mutex, typename _Predicate>
    void wait(unique_lock<_Mutex>& __lock, _Predicate __p) noexcept {
        while (!__p()) {
            wait(__lock);
        }
    }

private:
    mutable Spinlock mutex_;
    std::deque<std::pair<Scheduler*, Fiber::ptr>> waiters_;
};

}  // namespace flexy::fiber