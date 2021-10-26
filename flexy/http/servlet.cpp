#include "servlet.h"
#include <fnmatch.h>

namespace flexy::http {

FuncionServlet::FuncionServlet(callback&& cb)
: Servlet("FunctionServlet"), cb_(std::move(cb)) {
}

FuncionServlet::FuncionServlet(const callback& cb)
: Servlet("FunctionServlet"), cb_(cb) {
}

int32_t FuncionServlet::handle(const HttpRequest::ptr& request, 
                               const HttpResponse::ptr& response, const HttpSession& session) {
    return cb_(request, response, session);
}

ServletDispatch::ServletDispatch()
: Servlet("ServletDispatch"), default_(new NotFoundServlet) {

}

int32_t ServletDispatch::handle(const HttpRequest::ptr& request, 
                                const HttpResponse::ptr& response, const HttpSession& session) {
    auto&& slt = getMatchedServerlet(request->getPath());
    if (slt) {
        slt->handle(request, response, session);
    }
    return 0;
}

void addServlet(const std::string& uri, const Servlet::ptr& slt) {

}

void addServlet(const std::string& uri, FuncionServlet::callback&& cb) {

}

void addGlobServlet(const std::string& uri, const Servlet::ptr& slt) {

}

void delServletconst(const std::string& uri) {

}

void delGlobServlet(const std::string& uri) {

}


Servlet::ptr ServletDispatch::getServlet(const std::string& uri) const {
    READLOCK(mutex_);
    auto it = datas_.find(uri);
    return it == datas_.end() ? nullptr : it->second;
}

Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri) const {
    READLOCK(mutex_);
    for (const auto& [x, y] : globs_) {
        if (!fnmatch(x.c_str(), uri.c_str(), 0)) {
            return y;
        }
    }
    return nullptr;
}

Servlet::ptr ServletDispatch::getMatchedServerlet(const std::string& uri) const {
    READLOCK(mutex_);
    auto it = datas_.find(uri);
    if (it != datas_.end()) {
        return it->second;
    }
    for (const auto& [x, y] : globs_) {
        if (!fnmatch(x.c_str(), uri.c_str(), 0)) {
            return y;
        }
    }
    return default_;
}

NotFoundServlet::NotFoundServlet() : Servlet("NotFoundServlet") {

}

int32_t NotFoundServlet::handle(const HttpRequest::ptr& request, 
                                const HttpResponse::ptr& response, const HttpSession& session) {
    static const std::string BODY1 = "<html><head>\n"
                                     "<title>404 Not Found</title>\n"
                                     "</head><body>\n"
                                     "<h1>Not Found</h1>\n"
                                     "<p>The requested URL ";
    static const std::string BODY2 = " was not found on this server.</p>\n"
                                     "</body></html>";

    std::string RSP_BODY = BODY1 + request->getPath() + BODY2;
    response->setStatus(HttpStatus::NOT_FOUND);
    response->setHeader("Server", "flexy/1.0.0");
    response->setHeader("Content-type", "text/html");
    response->setBody(std::move(RSP_BODY));
    return 0;
}

}