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
    auto&& slt = getMatchedServlet(request->getPath());
    if (slt) {
        slt->handle(request, response, session);
    }
    return 0;
}

void ServletDispatch::addServlet(const std::string& uri, const Servlet::ptr& slt) {
    WRITELOCK(mutex_);
    datas_[uri] = slt;
}

void ServletDispatch::addServlet(const std::string& uri, FuncionServlet::callback&& cb) {
    WRITELOCK(mutex_);
    datas_[uri] = std::make_shared<FuncionServlet>(cb);
}

void ServletDispatch::addGlobServlet(const std::string& uri, const Servlet::ptr& slt) {
    WRITELOCK(mutex_);
    for (auto it = globs_.begin(); it != globs_.end(); ++it) {
        if (it->first == uri) {
            globs_.erase(it);
            break;
        }
    }
    globs_.emplace_back(uri, slt);
}


void ServletDispatch::addGlobServlet(const std::string& uri, FuncionServlet::callback&& cb) {
    WRITELOCK(mutex_);
    for (auto it = globs_.begin(); it != globs_.end(); ++it) {
        if (it->first == uri) {
            globs_.erase(it);
            break;
        }
    }
    globs_.emplace_back(uri, std::make_shared<FuncionServlet>(cb));
}

void ServletDispatch::delServlet(const std::string& uri) {
    WRITELOCK(mutex_);
    datas_.erase(uri);
}

void ServletDispatch::delGlobServlet(const std::string& uri) {
    WRITELOCK(mutex_);
    for (auto it = globs_.begin(); it != globs_.end(); ++it) {
        if (!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            globs_.erase(it);
            break;
        }
    }
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

Servlet::ptr ServletDispatch::getMatchedServlet(const std::string& uri) const {
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