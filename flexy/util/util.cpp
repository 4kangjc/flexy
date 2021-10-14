#include "util.h"
#include "flexy/thread/thread.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <chrono>
#include <memory>
#include <stdarg.h>
#include <charconv>

namespace flexy {

int GetThreadId() {
    return syscall(SYS_gettid);
}

std::string GetThreadName() {
    return Thread::GetName();
}

uint32_t GetThreadElapse() {
    return GetTimeMs() - Thread::GetStartTime();
}

uint32_t GetFiberId() {
    return 0;
}

uint64_t GetTimeUs() {
    auto p = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count();
}

uint64_t GetTimeMs() { return GetTimeUs() / 1000; }

uint64_t GetSteadyUs() {
    auto p = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count();
}

uint64_t GetSteadyMs() { return GetSteadyUs() / 1000; }

std::string format(const char* fmt, ...) {
    char buffer[500];
    std::unique_ptr<char[]> release1;
    char *base;
    for (int iter = 0; iter < 2; iter++) {
        int bufsize;
        if (iter == 0) {
            bufsize = sizeof(buffer);
            base = buffer;
        } else {
            bufsize = 30000;
            base = new char[bufsize];
            release1.reset(base);
        }
        char *p = base;
        char *limit = base + bufsize;
        if (p < limit) {
            va_list ap;
            va_start(ap, fmt);
            p += vsnprintf(p, limit - p, fmt, ap);
            va_end(ap);
        }
        // Truncate to available space if necessary
        if (p >= limit) {
            if (iter == 0) {
                continue;  // Try again with larger buffer
            } else {
                p = limit - 1;
                *p = '\0';
            }
        }
        break;
    }
    return base;
}

int64_t atoi(const char* begin, const char* end) {
    int64_t ans;
    std::from_chars(begin, end, ans);
    return ans;
}

}