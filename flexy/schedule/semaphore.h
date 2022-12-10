#pragma once

#include "flexy/schedule/fiber.h"
#include "flexy/thread/mutex.h"
#include "flexy/util/noncopyable.h"

#include <queue>

namespace flexy {

class Scheduler;

}  // namespace flexy

namespace flexy::fiber {

// fiber semaphore
class Semaphore : noncopyable {
public:
    Semaphore(size_t initial_councurrency = 0)
        : concurrency_(initial_councurrency) {}
    ~Semaphore();

    bool tryWait();
    void wait();
    void post();

private:
    mutable Spinlock mutex_;
    size_t concurrency_;
    std::queue<std::pair<Scheduler*, Fiber::ptr>> waiters_;
};

}  // namespace flexy::fiber