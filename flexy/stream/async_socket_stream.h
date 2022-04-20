#pragma once

#include "socket_stream.h"
#include "flexy/net/socket.h"
#include "flexy/schedule/iomanager.h"
#include "flexy/schedule/semaphore.h"
#include <memory>
#include <functional>
#include <deque>
#include <unordered_map>
#include <any>

namespace flexy {

class AsyncSockStream : public SockStream, public std::enable_shared_from_this<AsyncSockStream> {
public:
    using ptr = std::shared_ptr<AsyncSockStream>;
    using connect_callback = std::function<bool(const AsyncSockStream::ptr&)>;
    using disconnect_callback = std::function<void(const AsyncSockStream::ptr&)>;

    AsyncSockStream(const Socket::ptr& sock, bool owner = true);

    virtual bool start();
    virtual void close() override;

public:
    enum Error {
        OK = 0,
        TIMEOUT = -1,
        IO_ERROR = -2,
        NOT_CONNECT = -3
    };
protected:
    struct SendCtx {
        using ptr = std::shared_ptr<SendCtx>;
        virtual ~SendCtx() { }
        virtual bool doSend(const AsyncSockStream::ptr& stream) = 0;
    };

    struct Ctx : public SendCtx {
    public:
        using ptr = std::shared_ptr<Ctx>;
        virtual ~Ctx() { }
        Ctx();

        uint32_t sn;
        uint32_t timeout;
        uint32_t result;
        bool timed;

        Scheduler* scheduler;
        Fiber::ptr fiber;
        Timer::ptr timer;

        virtual void doRsp();
    };

public:
    void setWorker(IOManager* v) { worker_ = v; }
    IOManager* getWorker() const { return worker_; }

    void setIOManager(IOManager* v) { iomanager_ = v; }
    IOManager* getIOManager() const { return iomanager_; }

    bool isAutoConnect() const { return autoConnect_; }
    void setAutoConnect(bool v) { autoConnect_ = v; }

    const connect_callback& getConnectCb() const { return connectCb_; }
    const disconnect_callback& getDisconnectCb() const { return disconnectCb_; }

    void setConnectCb(const connect_callback& cb) { connectCb_ = cb; }
    void setConnectCb(connect_callback&& cb) { connectCb_ = std::move(cb); }
    void setDisconnectCb(const disconnect_callback& cb) { disconnectCb_ = cb; }
    void setDisconnectCb(disconnect_callback&& cb) { disconnectCb_ = std::move(cb); }

    template<class T>
    void setData(T&& v) { data_ = std::forward<T>(v); }

    template<class T>
    T getData() const {
        try {
            return std::any_cast<T>(data_);
        } catch (...) {

        }
        return T();
    }
protected:
    virtual void doRead();
    virtual void doWrite();
    virtual void startRead();
    virtual void startWrite();
    virtual void onTimeOut(const Ctx::ptr& ctx);
    virtual Ctx::ptr doRecv() = 0;
    virtual void onClose() { }

    Ctx::ptr getCtx(uint32_t sn);
    Ctx::ptr getAndDelCtx(uint32_t sn);

    template <class T>
    std::shared_ptr<T> getCtxAs(uint32_t sn) {
        auto&& ctx = getCtx(sn);
        if (ctx) {
            return std::dynamic_pointer_cast<T>(ctx);
        }
        return nullptr;
    }

    template <class T>
    std::shared_ptr<T> getAndDelCtxAs(uint32_t sn) {
        auto&& ctx = getAndDelCtx(sn);
        if (ctx) {
            return std::dynamic_pointer_cast<T>(ctx);
        }
        return nullptr;
    }

    bool addCtx(const Ctx::ptr& ctx);
    bool enqueue(const SendCtx::ptr& ctx);

    bool innerClose();
    bool waitFiber();

protected:
    fiber::Semaphore sem_;
    fiber::Semaphore waitSem_;
    rw_mutex queueMutex_;
    std::deque<SendCtx::ptr> queue_;
    rw_mutex mutex_;
    std::unordered_map<uint32_t, Ctx::ptr> ctxs_;

    uint32_t sn_;
    bool autoConnect_;
    // uint16_t tryConnectCount_;
    Timer::ptr timer_;
    IOManager* iomanager_;
    IOManager* worker_;

    connect_callback connectCb_;
    disconnect_callback disconnectCb_;

    std::any data_;

};

} // namespace flexy