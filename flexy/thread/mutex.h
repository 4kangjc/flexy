#pragma once

#include "flexy/util/noncopyable.h"

#include <mutex>
#include <shared_mutex>
#include <atomic>

#define LOCK_GUARD(x) std::lock_guard<decltype(x)> lk(x)

namespace flexy {

using rw_mutex = std::shared_mutex;

template <typename T>
using lock_guard = std::lock_guard<T>;

template <typename T>
using unique_lock = std::unique_lock<T>;

using ReadLock = std::shared_lock<rw_mutex>;
using WriteLock = std::lock_guard<rw_mutex>;            // 不支持手动 unlock
using WriteLock2 = std::unique_lock<rw_mutex>;          // 可手动 unlock

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


}