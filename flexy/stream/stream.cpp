#include "stream.h"
#include "flexy/util/log.h"

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

ssize_t Stream::readFixSize(void* buffer, size_t length) {
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        ssize_t len = read((char*)buffer + offset, left);
        if (len <= 0) {
            FLEXY_LOG_FMT_ERROR(g_logger, "readFixSize fail length = {} len = {} errno = {} errstr = {}",
                                length, len, errno, strerror(errno));
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

ssize_t Stream::readFixSize(const ByteArray::ptr& ba, size_t length) {
    size_t left = length;
    while (left > 0) {
        ssize_t len = read(ba, left);
        if (len <= 0) {
            FLEXY_LOG_FMT_ERROR(g_logger, "readFixSize fail length = {} len = {} errno = {} errstr = {}",
                                length, len, errno, strerror(errno));
            return len;
        }
        left -= len;
    }
    return length;
}

ssize_t Stream::writeFixSize(const void* buffer, size_t length) {
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        ssize_t len = write((char*)buffer + offset, left);
        if (len <= 0) {
            FLEXY_LOG_FMT_ERROR(g_logger, "writeFixSize fail length = {} len = {} errno = {} errstr = {}",
                                length, len, errno, strerror(errno));
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

ssize_t Stream::writeFixSize(const ByteArray::ptr& ba, size_t length) {
    size_t left = length;
    while (left > 0) {
        ssize_t len = read(ba, left);
        if (len <= 0) {
            FLEXY_LOG_FMT_ERROR(g_logger, "writeFixSize fail length = {} len = {} errno = {} errstr = {}",
                                length, len, errno, strerror(errno));
            return len;
        }
        left -= len;
    }
    return length;
}

} // namespace flexy