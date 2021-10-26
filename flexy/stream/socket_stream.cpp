#include "socket_stream.h"

namespace flexy {

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

int SockStream::read(void* buffer, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    return sock_->recv(buffer, length);
}

int SockStream::write(const void* buffer, size_t length) {
    if (!isConnected()) {
        return -1;
    }
    return sock_->send(buffer, length);
}

void SockStream::close() {
    if (sock_) {
        sock_->close();
    }
}


} // namespace flexy