#include "socket_stream.h"
#include "flexy/util/log.h"

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

SockStream::SockStream(const Socket::ptr& socket, bool owner)
: sock_(socket), owner_(owner) {

}

SockStream::~SockStream() {
    if (owner_ && sock_) {
        sock_->close();
    }
}

bool SockStream::isConnected() const {
    return sock_ && sock_->isConnected();
}

ssize_t SockStream::read(void* buffer, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    return sock_->recv(buffer, length);
}

ssize_t SockStream::read(const ByteArray::ptr &ba, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length);
    ssize_t rt = sock_->recv(iovs.data(), iovs.size());
    if (rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}

ssize_t SockStream::write(const void* buffer, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    return sock_->send(buffer, length);
}

ssize_t SockStream::write(const ByteArray::ptr &ba, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs, length);
    ssize_t rt = sock_->send(iovs.data(), iovs.size());
    if (rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    } else {
        FLEXY_LOG_FMT_ERROR(g_logger, "write fail length = {} errno = {}, {}",
                            length, errno, strerror(errno));
    }
    return rt;
}

void SockStream::close() {
    if (sock_) {
        sock_->close();
    }
}


} // namespace flexy