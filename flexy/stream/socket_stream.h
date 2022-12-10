#pragma once
#include "stream.h"
#include "flexy/net/socket.h"

namespace flexy {

class SockStream : public Stream {
public:
    using ptr = std::shared_ptr<SockStream>;
    SockStream(const Socket::ptr& sock, bool owner);
    ~SockStream();
    /*virtual*/ ssize_t read(void* buffer, size_t length) override;
    ssize_t read(const ByteArray::ptr& ba, size_t length) override;
    /*virtual*/ ssize_t write(const void* buffer, size_t length) override;
    ssize_t write(const ByteArray::ptr& ba, size_t length) override;
    virtual void close() override;

    auto& getSocket() const { return sock_; }
    bool isConnected() const;

    Address::ptr getLocalAddress() const {
        return sock_ ? sock_->getLocalAddress() : nullptr;
    }
    Address::ptr getRemoteAddress() const {
        return sock_ ? sock_->getRemoteAddress() : nullptr;
    }
    std::string getLocalAddressString() const {
        auto localAddress = getLocalAddress();
        return localAddress ? localAddress->toString() : "";
    }
    std::string getRemoteAddressString() const {
        auto remoteAddress = getRemoteAddress();
        return remoteAddress ? remoteAddress->toString() : "";
    }

protected:
    Socket::ptr sock_;
    bool owner_;
};

} // namespace flexy