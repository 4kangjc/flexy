#include "ws_server.h"
#include "flexy/util/log.h"

namespace flexy::http {

static auto g_logger = FLEXY_LOG_NAME("system");

WSServer::WSServer(IOManager* worker, IOManager* io_worker, IOManager* accept_worker) 
    : TcpServer(worker, io_worker, accept_worker), dispatch_(std::make_shared<WSServletDispatch>()) {
    type_ = "websocket_server";
}   

void WSServer::handleClient(const Socket::ptr& client) {
    FLEXY_LOG_DEBUG(g_logger) << *client;
    WSSession::ptr session(new WSSession(client));
    do {
        auto header = session->handleShake();
        if (!header) {
            FLEXY_LOG_DEBUG(g_logger) << "handleShake error";
            break;
        }
        auto servlet = dispatch_->getWSServlet(header->getPath());
        if (!servlet) {
            FLEXY_LOG_DEBUG(g_logger) << "no match WSServlet";
            break;
        }
        int rt = servlet->onConnect(header, session);
        if (rt) {
            FLEXY_LOG_DEBUG(g_logger) << "onConnect return " << rt;
            break;
        }

        while (true) {
            auto msg = session->recvMessage();
            if (!msg) {
                break;
            }
            rt = servlet->handle(header, msg, session);
            if (rt) {
                FLEXY_LOG_DEBUG(g_logger) << "handle return " << rt;
                break;
            }
        }

        servlet->onClose(header, session);
    } while (false);
    session->close();
}

} // namespace flexy::http