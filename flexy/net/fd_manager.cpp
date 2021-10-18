#include "fd_manager.h"
#include "hook.h"
#include "flexy/util/config.h"
#include "flexy/util/macro.h"

#include <sys/stat.h>

namespace flexy {

static auto g_FdCtx_init_size = Config::Lookup("fdctx.init.size", 64, "FdCtx vector init size");
static auto g_FdCtx_resize_times = Config::Lookup("fdctx.resize.times", 2.0f, "FdCtx vector resize times");

FdCtx::FdCtx(int fd) : isInit_(false), isSocket_(false), sysNonblock_(false),
    userNonblock_(false), isClosed_(false), fd_(fd), recvTimeout_(-1), sendTimeout_(-1) {
    FLEXY_ASSERT(init());
}

FdCtx::~FdCtx() = default;

bool FdCtx::init() {
    if (isInit_) {
        return true;
    }
    recvTimeout_ = -1;
    sendTimeout_ = -1;
    struct stat fd_stat;
    if (fstat(fd_, &fd_stat) == -1) {
        isInit_ = false;
        isSocket_ = false;
    } else {
        isInit_ = true;
        isSocket_ = S_ISSOCK(fd_stat.st_mode);
    }

    if (isSocket_) {
        int flags = fcntl_f(fd_, F_GETFL, 0);
        if (!(flags & O_NONBLOCK)) {
            fcntl_f(fd_, F_SETFL, flags | O_NONBLOCK);
        }
        sysNonblock_ = true;
    } else {
        sysNonblock_ = false;
    }
    userNonblock_ = false;
    isClosed_ = false;
    return isInit_;
}


void FdCtx::setTimeout(int type, uint64_t v) {
    if (type == SO_RCVTIMEO) {
        recvTimeout_ = v;
    } else {
        sendTimeout_ = v;
    }
}

uint64_t FdCtx::getTimeout(int type) {
    return type == SO_RCVTIMEO ? recvTimeout_ : sendTimeout_;
}

FdManager::FdManager() {
    datas_.resize(g_FdCtx_init_size->getValue());
}

FdManager::~FdManager() {
    for (auto ptr : datas_) {
        delete ptr;
    }
}

// 如果容器大小不大于fd并且auto_create为false, 返回nullptr
// 如果容器大小大于fd, 但没有初始化, 并且auto_create为false, 返回nullptr
FdCtx* FdManager::get(int fd, bool auto_create) {
    if (fd == -1) {
        return nullptr;
    }
    LOCK_GUARD(mutex_);
    if ((int)datas_.size() > fd) {
        if (datas_[fd]) {
            return datas_[fd];
        }
    }
    if (!auto_create) {
        return nullptr;
    }
    if (fd >= (int)datas_.size()) {
        datas_.resize(fd * g_FdCtx_resize_times->getValue());
    }
    datas_[fd] = new FdCtx(fd);
    return datas_[fd];
}


void FdManager::del(int fd) {
    LOCK_GUARD(mutex_);
    if ((int)datas_.size() <= fd) {
        return;
    }
    datas_[fd] = nullptr;
}

} // namespace flexy