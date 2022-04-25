#pragma once

#include "ws_session.h"
#include "servlet.h"
#include <type_traits>

namespace flexy::http {

class WSServlet : public Servlet {
public:
    using ptr = std::shared_ptr<WSServlet>;
    WSServlet(const std::string& name) : Servlet(name) {}
    virtual ~WSServlet() {}
    virtual int32_t handle(const HttpRequest::ptr& request, const HttpResponse::ptr& response, 
                           const SockStream::ptr& session) override { return 0; }
    virtual int32_t onConnect(const HttpRequest::ptr& header, const WSSession::ptr& session) = 0;
    virtual int32_t onClose(const HttpRequest::ptr& header, const WSSession::ptr& session) = 0;
    virtual int32_t handle(const HttpRequest::ptr& header, const WSFrameMessage::ptr& msg,
                           const WSSession::ptr& session) = 0;
};

class FunctionWSServlet : public WSServlet {
public:
    using ptr = std::shared_ptr<FunctionWSServlet>;
    using on_connect_cb = std::function<int32_t(const HttpRequest::ptr&, const WSSession::ptr&)>;
    using on_close_cb   = std::function<int32_t(const HttpRequest::ptr&, const WSSession::ptr&)>;
    using callback      = std::function<int32_t(const HttpRequest::ptr&, const WSFrameMessage::ptr&,
                                                const WSSession::ptr&)>;

    template <typename T>
    using enable_connect_close_cb = std::enable_if_t<std::is_convertible_v<T, on_connect_cb>>;

    template <typename T>
    using enable_callback = std::enable_if_t<std::is_convertible_v<T, callback>>;
    // FunctionWSServlet(const callback& cb, const on_connect_cb& connect_cb = nullptr, const on_close_cb& close_cb = nullptr);
    // FunctionWSServlet(callback&& cb, on_connect_cb&& connect_cb = nullptr, on_close_cb&& close_cb = nullptr);

    template <class Callback, class Connect = on_connect_cb, class Close = on_close_cb,
                typename = enable_callback<Callback>,
                typename = enable_connect_close_cb<Connect>,
                typename = enable_connect_close_cb<Close>>
    FunctionWSServlet(Callback&& cb, Connect&& connect_cb = nullptr, Close&& close_cb = nullptr)
                    : WSServlet("FunctionWSServlet"), callback_(std::forward<Callback>(cb)), 
                      onConnect_(std::forward<Connect>(connect_cb)),
                      onClose_(std::forward<Close>(close_cb)) { }

    virtual int32_t onConnect(const HttpRequest::ptr& header, const WSSession::ptr& session) override;
    virtual int32_t onClose(const HttpRequest::ptr& header, const WSSession::ptr& session) override;
    virtual int32_t handle(const HttpRequest::ptr& header, const WSFrameMessage::ptr& msg,
                           const WSSession::ptr& session) override;
private:
    callback callback_;
    on_connect_cb onConnect_;
    on_close_cb onClose_;
};

class WSServletDispatch : public ServletDispatch {
public:
    using ptr = std::shared_ptr<WSServletDispatch>;
    WSServletDispatch() { name_ = "WSServletDispatch"; }
    template <class Callback, class Connect = FunctionWSServlet::on_connect_cb, class Close = FunctionWSServlet::on_close_cb,
            typename = FunctionWSServlet::enable_callback<Callback>,
            typename = FunctionWSServlet::enable_connect_close_cb<Connect>,
            typename = FunctionWSServlet::enable_connect_close_cb<Close>>
    void addWSServlet(const std::string& uri, Callback&& cb, Connect&& connect_cb = nullptr, Close&& close_cb = nullptr) {
        ServletDispatch::addServlet(uri, std::make_shared<FunctionWSServlet>(
                            std::forward<Callback>(cb), std::forward<Connect>(connect_cb), 
                            std::forward<Close>(close_cb)));
    }

    template <class Callback, class Connect = FunctionWSServlet::on_connect_cb, class Close = FunctionWSServlet::on_close_cb,
            typename = FunctionWSServlet::enable_callback<Callback>,
            typename = FunctionWSServlet::enable_connect_close_cb<Connect>,
            typename = FunctionWSServlet::enable_connect_close_cb<Close>>
    void addGlobWSServlet(const std::string& uri, Callback&& cb, Connect&& connect_cb = nullptr, Close&& close_cb = nullptr) {
        ServletDispatch::addGlobServlet(uri, std::make_shared<FunctionWSServlet>(
                            std::forward<Callback>(cb), std::forward<Connect>(connect_cb), 
                            std::forward<Close>(close_cb)));
    }
    
    WSServlet::ptr getWSServlet(const std::string& uri);
};

} // namespace flexy::http