#pragma once

#include "flexy/net/tcp_server.h"
#include "ws_servlet.h"
#include "ws_session.h"

namespace flexy::http {

class WSServer : public TcpServer {
public:
    using ptr = std::shared_ptr<WSServer>;
    WSServer(IOManager* worker = IOManager::GetThis(),
             IOManager* io_worker = IOManager::GetThis(),
             IOManager* accept_worker = IOManager::GetThis());
    auto& getWSServletDispatch() const { return dispatch_; }
    void setWSServletDispatch(const WSServletDispatch::ptr& v) {
        dispatch_ = v;
    }

protected:
    virtual void handleClient(const Socket::ptr& client) override;

protected:
    WSServletDispatch::ptr dispatch_;
};

}  // namespace flexy::http