#include "async_socket_stream.h"
#include "flexy/util/log.h"
#include "flexy/util/macro.h"
#include "flexy/thread/atomic.h"


namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

AsyncSockStream::Ctx::Ctx() : sn(0), timeout(0), result(0),
                              timed(false), scheduler(nullptr) 
{ }

void AsyncSockStream::Ctx::doRsp() {
    Scheduler* scd = scheduler;
    if (!Atomic::compareAndSwapBool(scheduler, scd, (Scheduler*)nullptr)) {
        return;
    }
    if (!scd || !fiber) {
        return;
    }
    if (timer) {
        timer->cancel();
        timer = nullptr;
    }

    if (timed) {
        result = TIMEOUT;
    }
    scd->async(std::move(fiber));
}

AsyncSockStream::AsyncSockStream(const Socket::ptr& sock, bool owner) 
    : SockStream(sock, owner), waitSem_(2), sn_(0), autoConnect_(false), 
      /*tryConnectCount_(0),*/ iomanager_(nullptr), worker_(nullptr)
{ }

bool AsyncSockStream::start() {
    if (!iomanager_) {
        iomanager_ = IOManager::GetThis();
    } 
    if (!worker_) {
        worker_ = IOManager::GetThis();
    }

    waitFiber();

    if (timer_) {
        timer_->cancel();
        timer_ = nullptr;
    }

    if (!isConnected()) {
        if (!sock_->reconnect()) {
            innerClose();
            waitSem_.post();
            waitSem_.post();
            goto error;
        }
    }

    if (connectCb_) {
        if (!connectCb_(shared_from_this())) {
            innerClose();
            waitSem_.post();
            waitSem_.post();
            goto error;   
        }
    }

    startRead();
    startWrite();
    return true;

error:
    if (autoConnect_) {
        if (timer_) {
            timer_->cancel();
            timer_ = nullptr;
        }

        timer_ = iomanager_->addTimer(2 * 1000, &AsyncSockStream::start, shared_from_this());
    }

    return false;
}

void AsyncSockStream::doRead() {
    try {
        while (isConnected()) {
            auto ctx = doRecv();
            if (ctx) {
                ctx->doRsp();
            }
        }
    } catch (...) {
        // TODO log
    }

    FLEXY_LOG_DEBUG(g_logger) << "doRead out " << this;
    innerClose();
    waitSem_.post();

    if (autoConnect_) {
        iomanager_->addTimer(10, &AsyncSockStream::start, shared_from_this());
    }
}

void AsyncSockStream::doWrite() {
    try {
        while (isConnected()) {
            sem_.wait();
            decltype(queue_) ctxs;
            {
                WRITELOCK(queueMutex_);
                ctxs.swap(queue_);
            }
            auto self = shared_from_this();
            for (auto& ctx : ctxs) {
                if (!ctx->doSend(self)) {
                    innerClose();
                    break;
                }
            }
        }
    } catch (...) {
        // TODO
    }

    FLEXY_LOG_DEBUG(g_logger) << "doWrite out " << this;
    {
        WRITELOCK(queueMutex_);
        queue_.clear();
    }
    waitSem_.post();
}

void AsyncSockStream::startRead() {
    iomanager_->async(&AsyncSockStream::doRead, shared_from_this());
}

void AsyncSockStream::startWrite() {
    iomanager_->async(&AsyncSockStream::doWrite, shared_from_this());
}

void AsyncSockStream::onTimeOut(const Ctx::ptr& ctx) {
    FLEXY_LOG_DEBUG(g_logger) << "onTimeOut";
    {
        WRITELOCK(mutex_);
        ctxs_.erase(ctx->sn);
    }
    ctx->timed = true;
    ctx->doRsp();
}

AsyncSockStream::Ctx::ptr AsyncSockStream::getCtx(uint32_t sn) {
    READLOCK(mutex_);
    auto it = ctxs_.find(sn);
    return it != ctxs_.end() ? it->second : nullptr;
}

AsyncSockStream::Ctx::ptr AsyncSockStream::getAndDelCtx(uint32_t sn) {
    Ctx::ptr ctx;
    WRITELOCK(mutex_);
    if (auto it = ctxs_.find(sn); it != ctxs_.end()) {
        ctx = std::move(it->second);
        ctxs_.erase(it);
    }
    return ctx;
}

bool AsyncSockStream::addCtx(const Ctx::ptr& ctx) {
    WRITELOCK(mutex_);
    ctxs_[ctx->sn] = ctx;
    return true;
}

bool AsyncSockStream::enqueue(const SendCtx::ptr& ctx) {
    FLEXY_ASSERT(ctx);
    bool empty = false;
    {
        WRITELOCK(queueMutex_);
        empty = queue_.empty();
        queue_.push_back(ctx);
    }
    if (empty) {
        sem_.post();
    }
    return empty;
}

bool AsyncSockStream::innerClose() {
    FLEXY_ASSERT(iomanager_ == IOManager::GetThis());
    if (isConnected() && disconnectCb_) {
        disconnectCb_(shared_from_this());
    }
    onClose();
    SockStream::close();
    sem_.post();
    decltype(ctxs_) ctxs;
    {
        WRITELOCK(mutex_);
        ctxs.swap(ctxs_);
    }
    {
        WRITELOCK(queueMutex_);
        queue_.clear();
    }
    for (auto& [sn, ctx] : ctxs) {
        ctx->result = IO_ERROR;
        ctx->doRsp();
    }
    return true;
}

bool AsyncSockStream::waitFiber() {
    waitSem_.wait();
    waitSem_.wait();
    return true;
}

void AsyncSockStream::close() {
    autoConnect_ = false;
    SchedulerSwitcher ss(iomanager_);       // close函数切换到iomanager_协程调度器执行
    if (timer_) {
        timer_->cancel();
        timer_ = nullptr;
    }
    SockStream::close();
}
    
} // namespace flexy
