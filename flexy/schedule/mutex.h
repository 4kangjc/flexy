#pragma once

#include <atomic>
#include <deque>
#include "flexy/schedule/fiber.h"
#include "flexy/thread/mutex.h"
#include "flexy/util/noncopyable.h"

namespace flexy {

class Scheduler;

}  // namespace flexy

namespace flexy::fiber {

// Analogous to `std::mutex`, but it's for fiber.
class mutex : noncopyable {
public:
    mutex() = default;
    ~mutex();
    void lock();
    void unlock();

private:
    bool locked_ = false;
    mutable Spinlock mutex_;
    std::deque<std::pair<Scheduler*, Fiber::ptr>> waiters_;
};

}  // namespace flexy::fiber