#include <flexy/http/http_session.h>
#include <flexy/util/macro.h>
#include <flexy/net/socket.h>
#include <flexy/net/address.h>
#include <flexy/schedule/iomanager.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test() {
    auto socket = std::make_shared<flexy::Socket>(AF_INET, SOCK_STREAM);
    auto addr = flexy::Address::LookupAnyIPAddress("0.0.0.0:8020");
    FLEXY_ASSERT(socket->bind(addr));
    FLEXY_ASSERT(socket->listen());
    auto client = socket->accept();
    FLEXY_ASSERT(client);
    
    flexy::http::HttpSession hs(client);
    auto&& request = hs.recvRequest();
    if (request) {
        FLEXY_LOG_INFO(g_logger) << request->toString(); 
    }
}

int main() {
    // auto socket = std::make_shared<flexy::Socket>(AF_INET, SOCK_STREAM);
    // auto addr = flexy::Address::LookupAnyIPAddress("0.0.0.0:8020");
    // FLEXY_ASSERT(socket->bind(addr));
    // FLEXY_ASSERT(socket->listen());
    // auto client = socket->accept();
    // FLEXY_ASSERT(client);
    
    // flexy::http::HttpSession hs(client);
    // auto&& request = hs.recvRequest();
    // if (request) {
    //     FLEXY_LOG_INFO(g_logger) << *request;
    // }
    flexy::IOManager iom;
    go test;
}