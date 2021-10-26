#pragma once

#include "flexy/net/tcp_server.h"
#include "servlet.h"

namespace flexy::http {

class HttpServer : public TcpServer {
public:
    HttpServer(bool keepalive = false, IOManager* worker = IOManager::GetThis(),
               IOManager* io_worker = IOManager::GetThis(),
               IOManager* accept_worker = IOManager::GetThis());
    auto& getServletDispatch() const { return dispatch_; }
    bool isKeepalive() const { return isKeepalive_; }
protected:
    void handleClient(const Socket::ptr& client) override;
private:
    bool isKeepalive_;
    ServletDispatch::ptr dispatch_;
};

} // namespace flexy http