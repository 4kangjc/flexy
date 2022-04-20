#pragma once
#include "thread.h"
#include "flexy/net/hook.h"
#include <chrono>

namespace flexy::this_thread {

inline pid_t get_id() noexcept { return Thread::GetThreadId(); }

template <typename _Rep, typename _Period>
inline void sleep_for(const std::chrono::duration<_Rep, _Period>& __rtime) {
    if (__rtime <= __rtime.zero()) {
        return;
    }
    auto __s = std::chrono::duration_cast<std::chrono::seconds>(__rtime);
    auto __ns = std::chrono::duration_cast<std::chrono::nanoseconds>(__rtime - __s);
    struct ::timespec __ts = {
        static_cast<std::time_t>(__s.count()),
        static_cast<long>(__ns.count())
    };
    while (nanosleep_f(&__ts, &__ts) == -1 && errno == EINTR) // 使用hook前的nanosleep
    { }
}

template <typename _Clock, typename _Duration>
inline void sleep_until(const std::chrono::time_point<_Clock, _Duration>& __atime) {
#if __cplusplus > 201703L
	static_assert(chrono::is_clock_v<_Clock>);
#endif
    auto __now = _Clock::now();
    if (_Clock::is_steady) {
        if (__now < __atime) {
            sleep_for(__atime - __now);
        }
        return;
    }
    while (__now < __atime) {
        sleep_for(__atime - __now);
        __now = _Clock::now();
    }
    
}

inline void yield() noexcept { __gthread_yield(); }

} // namespace flexy this_thread