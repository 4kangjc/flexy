#include "http_server.h"
#include "http_session.h"
#include "flexy/util/log.h"

namespace flexy::http {

static auto g_logger = FLEXY_LOG_NAME("system");

[[deprecated]]
static void handleClientCb(const TcpServer::ptr self, const Socket::ptr& client) {
    auto thi = std::dynamic_pointer_cast<HttpServer>(self);
    // FLEXY_LOG_DEBUG(g_logger) << "handleClient " << *client;
    HttpSession session(client);
    do {
        auto&& req = session.recvRequest();
        if (!req) {
             FLEXY_LOG_DEBUG(g_logger) << "recv http request fail, error = "
            << errno << " errstr = " << strerror(errno) << " client = "
            << *client;
            break;
        }
        auto rsp = std::make_unique<HttpResponse>(req->getVesion(),
                                                  req->isClose() || !thi->isKeepalive());
        thi->getServletDispatch()->handle(req, rsp, session);
        session.sendResponse(rsp);
    } while (thi->isKeepalive());
    
    session.close();
}

void HttpServer::handleClient(const Socket::ptr& client) {
    HttpSession session(client);
    do {
        auto&& req = session.recvRequest();
        if (!req) {
             FLEXY_LOG_DEBUG(g_logger) << "recv http request fail, error = "
            << errno << " errstr = " << strerror(errno) << " client = "
            << *client;
            break;
        }
        auto rsp = std::make_unique<HttpResponse>(req->getVesion(),
                                                  req->isClose() || !isKeepalive_);
        dispatch_->handle(req, rsp, session);
        session.sendResponse(rsp);
    } while (isKeepalive_);
    
    session.close();
}

HttpServer::HttpServer(bool keepalive, IOManager* worker,
                       IOManager* io_worker, IOManager* accept_worker) :
        TcpServer(worker, io_worker, accept_worker), isKeepalive_(keepalive),
        dispatch_(new ServletDispatch) {
    // handleClient_ = handleClientCb;
}

} // namespace flexy http 