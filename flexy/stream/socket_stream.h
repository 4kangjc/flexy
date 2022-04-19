#pragma once
#include "stream.h"
#include "flexy/net/socket.h"

namespace flexy {

class SockStream : public Stream {
public:
    SockStream(const Socket::ptr& sock, bool owner);
    ~SockStream();
    /*virtual*/ ssize_t read(void* buffer, size_t length) override;
                ssize_t read(const ByteArray::ptr& ba, size_t length) override;
    /*virtual*/ ssize_t write(const void* buffer, size_t length) override;
                ssize_t write(const ByteArray::ptr& ba, size_t length)  override;
    virtual void close() override;

    auto& getSocket() const { return sock_; }
    bool isConnected() const;
protected:
    Socket::ptr sock_;
    bool owner_;
};

} // namespace flexy