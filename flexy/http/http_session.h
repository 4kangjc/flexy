#pragma once

#include "flexy/stream/socket_stream.h"
#include "http.h"

namespace flexy::http {

class HttpSession : public SockStream {
public:
    using ptr = std::shared_ptr<HttpSession>;
    HttpSession(const Socket::ptr& sock, bool owner = true);
    HttpRequest::ptr recvRequest();
    int sendResponse(const HttpResponse::ptr& rsp); 
};

}  // namespace flexy::http
