#include "http_session.h"
#include "http_parser.h"

namespace flexy::http {

HttpSession::HttpSession(const Socket::ptr& sock, bool owner) 
: SockStream(sock, owner) {
    
}

HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser parser;
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    std::unique_ptr<char[]> buffer(new char[buff_size]);

    char* data = buffer.get();
    int offset = 0;

    while (true) {
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            goto error;
        }
        len += offset;
        size_t nparse = parser.execute(data, len);
        if (parser.hasError()) {
            goto error;
        }
        offset = len - nparse;
        if (offset == (int)buff_size) {
            goto error;
        }
        if (parser.isFinished()) {
            goto body;
        }
    }
error:
    close();
    return nullptr;

body:
    int64_t length = parser.getContentLength();
    if (length > 0) {
        std::string body;
        body.resize(length);
        int len = std::min(length, (int64_t)offset);
        memcpy(body.data(), data, len);
        if (length > 0) {
            if (readFixSize(&body[len], length) <= 0) {
                goto error;
            }
        }
        parser.getData()->setBody(body);
    }
    parser.getData()->init();
    return std::move(parser.getData());
}

int HttpSession::sendResponse(const HttpResponse& rsp) {
    std::stringstream ss;
    ss << rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

} // namespace flexy http