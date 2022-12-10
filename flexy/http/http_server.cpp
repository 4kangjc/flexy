#include "http_server.h"
#include "http_session.h"
#include "flexy/util/log.h"

namespace flexy::http {

static auto g_logger = FLEXY_LOG_NAME("system");

void HttpServer::handleClient(const Socket::ptr& client) {
    // HttpSession::ptr session(new HttpSession(client));
    auto session = std::make_shared<HttpSession>(client);
    do {
        auto&& req = session->recvRequest();
        if (!req) {
             FLEXY_LOG_DEBUG(g_logger) << "recv http request fail, error = "
            << errno << " errstr = " << strerror(errno) << " client = "
            << *client;
            break;
        }
        auto rsp = std::make_unique<HttpResponse>(
            req->getVersion(), req->isClose() || !isKeepalive_);
        dispatch_->handle(req, rsp, session);
        session->sendResponse(rsp);
    } while (isKeepalive_);

    session->close();
}

HttpServer::HttpServer(bool keepalive, IOManager* worker, IOManager* io_worker,
                       IOManager* accept_worker)
    : TcpServer(worker, io_worker, accept_worker),
      isKeepalive_(keepalive),
      dispatch_(std::make_shared<ServletDispatch>()) {
    type_ = "http";
}

} // namespace flexy http 