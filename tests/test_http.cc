#include <flexy/http/http.h>
#include <flexy/util/log.h>
#include <iostream>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test_request() {
    auto req = std::make_shared<flexy::http::HttpRequest>();
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello flexy");

    req->dump(std::cout) << std::endl;
//    FLEXY_LOG_INFO(g_logger) << *req;
}

void test_response() {
    auto rsp = std::make_shared<flexy::http::HttpResponse>();
    rsp->setHeader("X-X", "flexy");
    rsp->setBody("hello flexy");
    rsp->setClose(false);
    rsp->setStatus((flexy::http::HttpStatus)400);

    rsp->dump(std::cout) << std::endl;
//    FLEXY_LOG_INFO(g_logger) << *rsp;
}

int main() {
    test_request();
    std::cout << std::endl;
    test_response();
}

