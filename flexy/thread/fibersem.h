#pragma once

#include "flexy/util/noncopyable.h"
#include "flexy/schedule/fiber.h"
#include "mutex.h"

#include <queue>

namespace flexy {

class Scheduler;

class FiberSemaphore : noncopyable {
public:
    FiberSemaphore(size_t initial_councurrency = 0)
    : concurrency_(initial_councurrency) {}
    ~FiberSemaphore();

    bool tryWait();
    void wait();
    void post();
private:
    mutable Spinlock mutex_;
    size_t concurrency_;
    std::queue<std::pair<Scheduler*, Fiber::ptr>> waiters_;
};

}  // namespace flexy