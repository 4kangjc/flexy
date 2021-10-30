#pragma once

#include "flexy/stream/socket_stream.h"
#include "http.h"
#include <optional>

namespace flexy::http {

class HttpSession : public SockStream {
public:
    HttpSession(const Socket::ptr& sock, bool owner = true);
    HttpRequest::ptr recvRequest();
    int sendResponse(const HttpResponse::ptr& rsp); 
};

} // namespace flexy http