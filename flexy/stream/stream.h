#pragma once
#include <unistd.h>

namespace flexy {

class Stream {
public:
    virtual ~Stream() { }
    virtual int read(void* buffer, size_t length) = 0;

    virtual int readFixSize(void* buffer, size_t length);

    virtual int write(const void* buffer, size_t length) = 0;

    virtual int writeFixSize(const void* buffer, size_t length);

    virtual void close() = 0;
};

} // namespace flexy