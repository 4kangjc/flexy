#pragma once

#include <pthread.h>

namespace flexy {

class Atomic {
public:
    // 将 v 加到 t 上，结果更新到 t，并返回操作之后新 t 的值
    template <typename T, class S = T>
    static T addFetch(volatile T& t, S v = 1) {
        return __sync_add_and_fetch(&t, (T)v);
    }
    // 从 t 减去 v，结果更新到 t，并返回操作之后新 t 的值
    template <typename T, class S = T>
    static T subFetch(volatile T& t, S v = 1) {
        return __sync_sub_and_fetch(&t, (T)v);
    }
    // 将 t 与 v 相或， 结果更新到 t，并返回操作之后新 t的值
    template <typename T, class S = T>
    static T orFetch(volatile T& t, S v) {
        return __sync_or_and_fetch(&s, (T)v);
    }
    // 将 t 与 v 相与，结果更新到 t ，并返回操作之后新 t的值
    template<class T, class S>
    static T andFetch(volatile T& t, S v) {
        return __sync_and_and_fetch(&t, (T)v);
    }
    // 将 t 与 v 异或，结果更新到 t ，并返回操作之后新 t 的值
    template<class T, class S>
    static T xorFetch(volatile T& t, S v) {
        return __sync_xor_and_fetch(&t, (T)v);
    }
    // 将 v 取反后，与 v 相与，结果更新到 t，并返回操作之后新 t 的值
    template<class T, class S>
    static T nandFetch(volatile T& t, S v) {
        return __sync_nand_and_fetch(&t, (T)v);
    }
    // 将 v 加到 t 上，结果更新到 t，并返回操作之前 t 的值
    template<class T, class S>
    static T fetchAdd(volatile T& t, S v = 1) {
        return __sync_fetch_and_add(&t, (T)v);
    }
    // 从 t 减去 v，结果更新到 t，并返回操作之前 t 的值
    template<class T, class S>
    static T fetchSub(volatile T& t, S v = 1) {
        return __sync_fetch_and_sub(&t, (T)v);
    }
    // 将 t 与 v 相或，结果更新到 t， 并返回操作之前t的值
    template<class T, class S>
    static T fetchOr(volatile T& t, S v) {
        return __sync_fetch_and_or(&t, (T)v);
    }
    // 将 t 与 v 相与，结果更新到 t，并返回操作之前 t 的值
    template<class T, class S>
    static T fetchAnd(volatile T& t, S v) {
        return __sync_fetch_and_and(&t, (T)v);
    }
    // 将 t 与 v 异或，结果更新到 t，并返回操作之前 t 的值
    template<class T, class S>
    static T fetchXor(volatile T& t, S v) {
        return __sync_fetch_and_xor(&t, (T)v);
    }
    // 将 t 取反后，与 v 相与，结果更新到 t，并返回操作之前 t 的值
    template<class T, class S>
    static T fetchNand(volatile T& t, S v) {
        return __sync_fetch_and_nand(&t, (T)v);
    }
    // 比较 t 和 old_val的值，如果两者相等，则将new_val更新到 t 并返回操作之前的 t
    template<class T, class S>
    static T compareAndSwap(volatile T& t, S&& old_val, S&& new_val) {
        return __sync_val_compare_and_swap(&t, (T)old_val, (T)new_val);
    }
    // 比较 t 和 old_val的值，如果两者相等，则将new_val更新到 t 并返回true
    template <typename T, class S>
    static bool compareAndSwapBool(volatile T& t, S&& old_val, S&& new_val) {
        return __sync__bool_compare_and_swap(&t, (T)old_val, (T)new_val);
    }
};

}