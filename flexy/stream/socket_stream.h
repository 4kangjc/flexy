#pragma once
#include "stream.h"
#include "flexy/net/socket.h"

namespace flexy {

class SockStream : public Stream {
public:
    SockStream(const Socket::ptr& sock, bool owner);
    ~SockStream();
    int read(void* buffer, size_t length) override;
    
    int write(const void* buffer, size_t length) override;

    void close() override;

    auto& getSocket() const { return sock_; }
    bool isConnected() const;
protected:
    Socket::ptr sock_;
    bool owner_;
};

} // namespace flexy