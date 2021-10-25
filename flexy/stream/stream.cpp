#include "stream.h"

namespace flexy {

int Stream::readFixSize(void* buffer, size_t length) {
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        size_t len = read((char*)buffer + offset, left);
        if (len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int Stream::writeFixSize(const void* buffer, size_t length) {
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        size_t len = write((char*)buffer + offset, left);
        if (len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

} // namespace flexy