#include "flexy/http/ws_server.h"
#include "flexy/util/log.h"

static auto& g_logger = FLEXY_LOG_ROOT();

void run() {
    auto server = std::make_shared<flexy::http::WSServer>();
    auto addr = flexy::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if (!addr) {
        FLEXY_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    auto fun = [](const flexy::http::HttpRequest::ptr& header,
                  const flexy::http::WSFrameMessage::ptr& msg,
                  const flexy::http::WSSession::ptr& session) {
        session->sendMessage(msg);
        return 0;
    };

    server->getWSServletDispatch()->addWSServlet("/flexy", fun);
    while (!server->bind(addr)) {
        FLEXY_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    server->start();
}

int main() {
    flexy::IOManager iom(2);
    go run;
}
