#include <flexy/http/http_parser.h>
#include <flexy/util/log.h>
#include <flexy/net/address.h>
#include <flexy/net/socket.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

const char test_request_data[] = "POST / HTTP/1.1\r\n" 
                                 "Host: www.baidu.com\r\n"
                                 "Content-Length: 10\r\n\r\n"
                                 "1234567890";

void test_request() {
    flexy::http::HttpRequestParser parser;
    std::string tmp = test_request_data;
    char* begin = tmp.data();
    size_t s = parser.execute(begin, tmp.size());
    FLEXY_LOG_INFO(g_logger) << "execute rt = " << s << " has_error = "
    << parser.hasError() << " is_finnished = " << parser.isFinished() 
    << " content_length = " << parser.getContentLength() << ", " << tmp.size() - s;

    tmp.resize(tmp.size() - s);
    FLEXY_LOG_INFO(g_logger) << parser.getData();
    FLEXY_LOG_INFO(g_logger) << begin;
}

void test_response() {
    flexy::http::HttpResponseParser parser;
    auto addr = flexy::Address::LookupAnyIPAddress("www.baidu.com:80", AF_UNSPEC);
    auto sock = flexy::Socket::CreateTCP(addr->getFamily());
    sock->connect(addr);
    const char buff[] = "GET / HTTP/1.1\r\n\r\n";
    sock->send(buff);
    std::string buf;
    buf.resize(1000);
    int offset = 0;
    char* data = buf.data();
    while (true) {
        int len = sock->recv(data + offset, 1000 - offset);
        // FLEXY_LOG_INFO(g_logger) << data;
        if (len <= 0) {
            sock->close();
            FLEXY_LOG_ERROR(g_logger) << "close";
            return;
        }
        len += offset;
        size_t nparse = parser.execute(data, len, false);
        FLEXY_LOG_INFO(g_logger) << "execute rt = " << nparse;
        if (parser.hasError()) {
            FLEXY_LOG_ERROR(g_logger) << "has error";
            sock->close();
            return;
        }
        offset = len - nparse;
        if (parser.isFinished()) {
            FLEXY_LOG_INFO(g_logger) << "finish";
            break;
        }
    }
    FLEXY_LOG_INFO(g_logger) << parser.getData();
}

int main() {
    test_request();
    test_response();
}