#pragma once
#include <string_view>
#include <functional>
#include "http_session.h"
#include "flexy/thread/mutex.h"

namespace flexy::http {

class Servlet {
public:
    using ptr = std::shared_ptr<Servlet>;
    Servlet(std::string_view name) : name_(name) {}
    virtual ~Servlet() {}
    virtual int32_t handle(const HttpRequest::ptr& request,
                           const HttpResponse::ptr& response,
                           const SockStream::ptr& sesion) = 0;
    auto& getName() const { return name_; }

protected:
    std::string name_;
};

class FuncionServlet : public Servlet {
public:
    using callback =
        std::function<int32_t(const HttpRequest::ptr&, const HttpResponse::ptr&,
                              const SockStream::ptr&)>;
    FuncionServlet(callback&& cb);
    FuncionServlet(const callback& cb);
    int32_t handle(const HttpRequest::ptr& request,
                   const HttpResponse::ptr& response,
                   const SockStream::ptr& session) override;

private:
    callback cb_;
};

class ServletDispatch : public Servlet {
public:
    using ptr = std::shared_ptr<ServletDispatch>;
    ServletDispatch();
    int32_t handle(const HttpRequest::ptr& request,
                   const HttpResponse::ptr& response,
                   const SockStream::ptr& session) override;
    void addServlet(const std::string& uri, const Servlet::ptr& slt);
    void addServlet(const std::string& uri, FuncionServlet::callback&& cb);
    void addGlobServlet(const std::string& uri, const Servlet::ptr& slt);
    void addGlobServlet(const std::string& uri, FuncionServlet::callback&& slt);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    auto& getDefault() const { return default_; }
    void setDefault(Servlet::ptr&& v) { default_ = std::move(v); }

    Servlet::ptr getServlet(const std::string& uri) const;
    Servlet::ptr getGlobServlet(const std::string& uri) const;
    Servlet::ptr getMatchedServlet(const std::string& uri) const;

private:
    mutable rw_mutex mutex_;
    std::unordered_map<std::string, Servlet::ptr> datas_;    
    std::vector<std::pair<std::string, Servlet::ptr>> globs_;
    Servlet::ptr default_;
};

class NotFoundServlet : public Servlet {
public:
    NotFoundServlet();
    int32_t handle(const HttpRequest::ptr& request,
                   const HttpResponse::ptr& response,
                   const SockStream::ptr& session) override;
};
} // namespace flexy http