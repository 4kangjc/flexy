#pragma once

#if defined  __GNUC__ || defined __llvm__ 
// 告诉编译器优化,条件大概率成立
#   define FLEXY_LIKELY(x)       __builtin_expect(!!(x), 1)
// 告诉编译器优化,条件大概率不成立
#   define FLEXY_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define FLEXY_LIKELY(x)          (x)
#   define FLEXY_UNLIKELY(x)        (x)
#endif