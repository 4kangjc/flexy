#pragma once

#include <byteswap.h>
#include <stdint.h>

#define FLEXY_LITTLE_ENDIAN 1
#define FLEXY_BIG_ENDIAN 2

#if BYTE_ORDER == BIG_ENDIAN 
#define FLEXY_BYTE_ORDER FLEXY_BIG_ENDIAN
#else
#define FLEXY_BYTE_ORDER FLEXY_LITTLE_ENDIAN
#endif


namespace flexy {

template <typename T>
T bswap(T value) {
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        return (T)bswap_64((uint64_t)value);
    } else if constexpr (sizeof(T) == sizeof(uint32_t)) {
        return (T)bswap_32((uint32_t)value);
    } else if constexpr (sizeof(T) == sizeof(uint16_t)) {
        return (T)bswap_16((uint16_t)value);
    } else {

    }
}

#if FLEXY_BYTE_ORDER == FLEXY_BIG_ENDIAN
template <typename T>
T byteswap(T t) {
    return t;
}

#else
template <typename T>
T byteswap(T t) {
    return bswap(t);
}
#endif

} // namespace flexy
