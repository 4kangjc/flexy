#pragma once

#include <cassert>
#include "likely.h"
#include "log.h"
#include "util.h"

inline auto __assert_logger_ = FLEXY_LOG_NAME("system");

#define FLEXY_ASSERT(x)                                  \
    if (FLEXY_UNLIKELY(!(x))) {                          \
        FLEXY_LOG_ERROR(__assert_logger_)                \
            << "ASSERTION: " << #x << "\nbacktrace:\n"   \
            << flexy::BacktraceToString(100, 2, "    "); \
        assert(x);                                       \
    }

#define FLEXY_ASSERT2(x, w)                              \
    if (FLEXY_UNLIKELY((!(x)))) {                        \
        FLEXY_LOG_ERROR(__assert_logger_)                \
            << "ASSERTION: " << #x << '\n'               \
            << w << "\nbacktrace:\n"                     \
            << flexy::BacktraceToString(100, 2, "    "); \
        assert(x);                                       \
    }
