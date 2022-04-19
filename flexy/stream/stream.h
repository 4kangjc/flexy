#pragma once
#include <unistd.h>
#include "flexy/net/bytearray.h"

namespace flexy {

class Stream {
public:
    virtual ~Stream() = default;
    virtual ssize_t read(void* buffer, size_t length) = 0;
    virtual ssize_t read(const ByteArray::ptr& ba, size_t length) = 0;
    virtual ssize_t readFixSize(void* buffer, size_t length);
    virtual ssize_t readFixSize(const ByteArray::ptr& ba, size_t length);
    virtual ssize_t write(const void* buffer, size_t length) = 0;
    virtual ssize_t write(const ByteArray::ptr& ba, size_t length) = 0;
    virtual ssize_t writeFixSize(const void* buffer, size_t length);
    virtual ssize_t writeFixSize(const ByteArray::ptr& ba, size_t length);
    virtual void close() = 0;
};

} // namespace flexy