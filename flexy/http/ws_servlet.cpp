#include "ws_servlet.h"

namespace flexy::http {

int32_t FunctionWSServlet::onConnect(const HttpRequest::ptr& header, const WSSession& session) {
    if (onConnect_) {
        return onConnect_(header, session);
    }
    return 0;
}

int32_t FunctionWSServlet::onClose(const HttpRequest::ptr& header, const WSSession& session) {
    if (onClose_) {
        return onClose_(header, session);
    }
    return 0;
}

int32_t FunctionWSServlet::handle(const HttpRequest::ptr& header, const WSFrameMessage::ptr& msg,
               const WSSession& session) {
    if (callback_) {
        return callback_(header, msg, session);
    }
    return 0;
}

WSServlet::ptr WSServletDispatch::getWSServlet(const std::string& uri) {
    auto slt = getMatchedServlet(uri);
    return std::dynamic_pointer_cast<WSServlet>(slt);
}

} // namespace flexy::http