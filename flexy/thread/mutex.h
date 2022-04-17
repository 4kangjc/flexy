#pragma once

#include "flexy/util/noncopyable.h"

#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <tbb/spin_rw_mutex.h>

#define LOCK_GUARD(x) std::lock_guard<decltype(x)> lk(x)

namespace flexy {

using rw_mutex = std::shared_mutex;
using spin_rw_mutex = tbb::spin_rw_mutex;

template <typename T>
using lock_guard = std::lock_guard<T>;

template <typename T>
using unique_lock = std::unique_lock<T>;

template <typename T>
using ReadLock = std::shared_lock<T>;
template <typename T>
using WriteLock = std::lock_guard<T>;            // 不支持手动 unlock
template <typename T>
using WriteLock2 = std::unique_lock<T>;          // 可手动 unlock

#define READLOCK(x)   ReadLock<std::decay_t<decltype(x)>>   lk(x)
#define WRITELOCK(x)  WriteLock<decltype(x)>                lk(x)

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