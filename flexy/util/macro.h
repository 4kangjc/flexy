#pragma once

#include "log.h"
#include "util.h"
#include <cassert>

#if defined  __GNUC__ || defined __llvm__ 
#   define FLEXY_LIKELY(x)       __builtin_expect(!!(x), 1)
#   define FLEXY_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define FLEXY_LIKELY(x)          (x)
#   define FLEXY_UNLIKELY(x)        (x)
#endif

#define FLEXY_ASSERT(x) \
    if (FLEXY_UNLIKELY(!(x))) { \
        FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "ASSERTION: " << #x \
            << "\nbacktrace:\n" << flexy::BacktraceToString(100, 2, "    ");  \
        assert(x); \
    }

#define FLEXY_ASSERT2(x, w) \
    if (FLEXY_UNLIKELY((!(x)))) { \
        FLEXY_LOG_ERROR(FLEXY_LOG_ROOT()) << "ASSERTION: " << #x << '\n' << w   \
            << "\nbacktrace:\n" << flexy::BacktraceToString(100, 2, "    ");  \
        assert(x); \
    }
    