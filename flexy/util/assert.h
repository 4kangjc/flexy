#pragma once

#include <cassert>
#include "log.h"
#include "util.h"
#include "likely.h"

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

