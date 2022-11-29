#pragma once

#include "flexy/util/noncopyable.h"

#include <mutex>
#include <shared_mutex>
#include <atomic>

#if __cpp_deduction_guides >= 201606
#define LOCK_GUARD(x) std::lock_guard lk(x)
#else
#define LOCK_GUARD(x) std::lock_guard<std::decay_t<decltype(x)>> lk(x)
#endif

namespace flexy {

using rw_mutex = std::shared_mutex;
using spin_rw_mutex = rw_mutex;

template <typename T>
using lock_guard = std::lock_guard<T>;

template <typename T>
using unique_lock = std::unique_lock<T>;

template <typename... T>
using scoped_lock = std::scoped_lock<T...>;

template <typename T>
using ReadLock = std::shared_lock<T>;
template <typename T>
using WriteLock = std::lock_guard<T>;            // 不支持手动 unlock
template <typename T>
using WriteLock2 = std::unique_lock<T>;          // 可手动 unlock

#if __cplusplus > 201703L
#define READLOCK(x)   ReadLock   lk(x)          // cpp20 类别名模板实参推导
#define WRITELOCK(x)  WriteLock  lk(x)
#define WRITELOCK2(x) WriteLock2 lk(x)
#elif __cpp_deduction_guides >= 201606
#define READLOCK(x)   std::shared_lock  lk(x)   // cpp17 类模板实参推导 (CTAD)
#define WRITELOCK(x)  std::lock_guard   lk(x)
#define WRITELOCK2(x) std::unique_lock  lk(x)
#else
#define READLOCK(x)   ReadLock<std::decay_t<decltype(x)>>   lk(x)
#define WRITELOCK(x)  WriteLock<std::decay_t<decltype(x)>>  lk(x)
#define WRITELOCK2(x) WriteLock2<std::decay_t<decltype(x)>> lk(x)
#endif

// TODO: boost::upgrade_lock 可从读锁直接升级为写锁

using mutex = std::mutex;

class NullMutex : noncopyable {
public:
    NullMutex() = default;
    ~NullMutex() = default;
    void lock() {}
    void unlock() {}
};

class Spinlock : noncopyable {
public:
    Spinlock() {
        pthread_spin_init(&mutex_, 0);
    }
    ~Spinlock() {
        pthread_spin_destroy(&mutex_);
    }
    void lock() {
        pthread_spin_lock(&mutex_);
    }
    void unlock() {
        pthread_spin_unlock(&mutex_);
    }
private:
    pthread_spinlock_t mutex_;
};

class CASlock : noncopyable {
public:
    CASlock() = default;
    ~CASlock() = default;
    void lock() {
        while (m_mutex.test_and_set(std::memory_order_acquire));
    }
    void unlock() {
        m_mutex.clear(std::memory_order_release);
    }
private:
    std::atomic_flag m_mutex = ATOMIC_FLAG_INIT;
};


}
