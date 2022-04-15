#pragma once

#include "flexy/util/noncopyable.h"
#include "flexy/schedule/fiber.h"
#include "mutex.h"
#include <deque>
#include <atomic>

namespace flexy {

class Scheduler;

class FiberMutex : noncopyable {
public:
    FiberMutex() = default;
    ~FiberMutex();
    void lock();
    void unlock();
private:
    bool locked_ = false;
    mutable mutex mutex_;
    std::deque<std::pair<Scheduler*, Fiber::ptr>> waiters_;
};

} // namespace flexy