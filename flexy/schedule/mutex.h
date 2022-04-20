#pragma once

#include "flexy/util/noncopyable.h"
#include "flexy/schedule/fiber.h"
#include "flexy/thread/mutex.h"
#include <deque>
#include <atomic>

namespace flexy {

class Scheduler;

} // namespace flexy

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


} // namespace flexy fiber