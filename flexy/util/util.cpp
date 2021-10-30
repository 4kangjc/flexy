#include "util.h"
#include "flexy/thread/thread.h"
#include "flexy/schedule/fiber.h"
#include "log.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <chrono>
#include <memory>
#include <stdarg.h>
#include <charconv>
#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>

static auto g_logger = FLEXY_LOG_NAME("system");

namespace flexy {

int GetThreadId() {
    return syscall(SYS_gettid);
}

const std::string& GetThreadName() {
    return Thread::GetName();
}

uint32_t GetThreadElapse() {
    return GetTimeMs() - Thread::GetStartTime();
}

uint32_t GetFiberId() {
    return Fiber::GetFiberId();
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

std::string TimeToStr(time_t ts, const std::string& fmt) {
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), fmt.c_str(), &tm);
    return buf;
}

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
    int64_t ans = 0;
    std::from_chars(begin, end, ans);
    return ans;
}

static std::string damangle(const char* str) {
    size_t size = 0;
    int status = 0;
    std::string rt;
    rt.resize(256);
    if (1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", rt.data())) {
        char* v = abi::__cxa_demangle(rt.data(), nullptr, &size, &status);
        if (v) {
            std::string result(v);
            free(v);
            return result;
        } 
    }
    if (1 == sscanf(str, "%255s", rt.data())) {
        return rt;
    }
    return str;
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    auto array = (void**)malloc(size * sizeof(void*));
    size_t s = ::backtrace(array, size);
    char** strings = backtrace_symbols(array, s);
    if (strings == NULL) {
        FLEXY_LOG_ERROR(g_logger) << "backtrace_symbols eroor";
        return;
    }
    for (size_t i = skip; i < s; ++i) {
        // bt.push_back(strings[i]);
        bt.push_back(damangle(strings[i]));
    }
    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for (size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

}