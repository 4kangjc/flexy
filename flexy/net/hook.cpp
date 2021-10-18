#include "hook.h"
#include "fd_manager.h"
#include "flexy/schedule/iomanager.h"
#include "flexy/util/config.h"

#include <dlfcn.h>
#include <stdarg.h>

static auto g_logger = FLEXY_LOG_NAME("system");

namespace flexy {

static thread_local bool t_hook_enable = false;
static auto g_tcp_connect_timeout = Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

#define HOOK_FUN(XX)        \
    XX(sleep)               \
    XX(usleep)              \
    XX(nanosleep)           \
    XX(socket)              \
    XX(connect)             \
    XX(accept)              \
    XX(read)                \
    XX(readv)               \
    XX(recv)                \
    XX(recvfrom)            \
    XX(recvmsg)             \
    XX(write)               \
    XX(writev)              \
    XX(send)                \
    XX(sendto)              \
    XX(sendmsg)             \
    XX(close)               \
    XX(fcntl)               \
    XX(ioctl)               \
    XX(getsockopt)          \
    XX(setsockopt)         

void hook_init() {
    static bool is_inited = false;
    if (is_inited) {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
    is_inited = true;
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();
        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) {
            FLEXY_LOG_INFO(g_logger) << "tcp connect tinmeout change from " << old_value << " to " << new_value;
            s_connect_timeout = new_value;
        });
    };
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag; 
}

} // namespace flexy


struct timer_info {
    int cacelled = 0;
};

template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun&& func, const char* hook_fun_name, uint32_t event,
                     int timeout_type, Args&&... args) {
    if (!flexy::t_hook_enable) {
        if (!func) {
            flexy::hook_init();
        }
        return func(fd, std::forward<Args>(args)...);
    }
    auto ctx = flexy::FdMsg::GetInstance().get(fd);
    if (!ctx) {
        return func(fd, std::forward<Args>(args)...);
    }
    if (ctx->isClose()) {
        errno = EBADF;
        return -1;
    }
    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return func(fd, std::forward<Args>(args)...);
    }

    uint64_t timeout = ctx->getTimeout(timeout_type);
    auto tinfo = std::make_shared<timer_info>();
retry:
    ssize_t n = func(fd, std::forward<Args>(args)...);
    while (n == -1 && errno == EINTR) {
        n = func(fd, std::forward<Args>(args)...);
    }
    if (n == -1 && errno == EAGAIN) {
        auto iom = flexy::IOManager::GetThis();
        flexy::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);
        if (timeout != ~0ull) {
            timer = iom->addTimer(timeout, [winfo, fd, iom, event]() {
                auto t = winfo.lock();
                if (!t || t->cacelled) {
                    return;
                }
                t->cacelled = ETIMEDOUT;
                iom->cancelEvent(fd, (flexy::Event)event);
            });
        }

        int rt = iom->addEvent(fd, (flexy::Event)event);
        if (!rt) {
            FLEXY_LOG_ERROR(g_logger) << hook_fun_name << " addEvent(" 
                << fd << ", " << (flexy::Event)event << ")";
            if (timer) {
                timer->cacel();
            }
            return -1;
        } else {
            flexy::Fiber::Yield();
            if (timer) {
                timer->cacel();
            }
            if (tinfo->cacelled) {
                errno = tinfo->cacelled;
                return -1;
            }
            goto retry;
        }
    }
    return n;
}

extern "C" {

#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if (!flexy::t_hook_enable) {
        if (!sleep_f) {
            flexy::hook_init();
        }
        return sleep_f(seconds);
    }
    auto fiber = flexy::Fiber::GetThis();
    auto iom = flexy::IOManager::GetThis();
    iom->addTimer(seconds * 1000, [iom, fiber]() {
        iom->async(fiber);
    });
    flexy::Fiber::Yield(); 
    return 0;
}

int usleep(useconds_t usec) {
    if (!flexy::t_hook_enable) {
        if (!usleep_f) {
            flexy::hook_init();
        }
        return usleep_f(usec);
    }
    auto fiber = flexy::Fiber::GetThis();
    auto iom = flexy::IOManager::GetThis();
    iom->addTimer(usec / 1000, [iom, fiber]() {
        iom->async(fiber);
    });
    flexy::Fiber::Yield(); 
    return 0;
}

int nanosleep(const struct timespec* req, struct timespec* rem) {
    if (!flexy::t_hook_enable) {
        if (!nanosleep_f) {
            flexy::hook_init();
        }
        return nanosleep_f(req, rem);
    }
    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    auto fiber = flexy::Fiber::GetThis();
    auto iom = flexy::IOManager::GetThis();
    iom->addTimer(timeout_ms, [iom, fiber]() {
        iom->async(fiber);
    });
    flexy::Fiber::Yield(); 
    return 0;
}

int socket(int domain, int type, int protocol) {
    if (!flexy::t_hook_enable) {
        if (!socket_f) {
            flexy::hook_init();
        }
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if (fd == -1) {
        return -1;
    }
    flexy::FdMsg::GetInstance().get(fd, true);
    return fd;
}

int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    return connect_f(sockfd, addr, addrlen);        // TODO
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, flexy::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int fd = do_io(sockfd, accept_f, "accept", flexy::READ, SO_RCVTIMEO, addr, addrlen);
    if (fd >= 0) {
        flexy::FdMsg::GetInstance().get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void* buf, size_t count) {
    return do_io(fd, read_f, "read", flexy::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec* iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", flexy::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", flexy::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", flexy::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", flexy::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void* buf, size_t count) {
    return do_io(fd, write_f, "write", flexy::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", flexy::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
    return do_io(sockfd, send_f, "send", flexy::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen) {
    return do_io(sockfd, sendto_f, "sendto", flexy::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr* msg, int flags) {
    return do_io(sockfd, sendmsg_f, "sendmsg", flexy::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if (!flexy::t_hook_enable) {
        if (!close_f) {
            flexy::hook_init();
        }
        return close_f(fd);
    }
    auto ctx = flexy::FdMsg::GetInstance().get(fd);
    if (ctx) {
        auto iom = flexy::IOManager::GetThis();
        if (iom) {
            iom->cancelAll(fd);
        }
        flexy::FdMsg::GetInstance().del(fd);
    }
    return close_f(fd);
}

int fctnl(int fd, int cmd, ...) {
    if (!flexy::t_hook_enable) {
        if (!fcntl_f) {
            flexy::hook_init();
        }
    }
    va_list va;
    va_start(va, cmd);
    switch (cmd) {
        case F_SETFL: {
            int arg = va_arg(va, int);
            va_end(va);
            auto ctx = flexy::FdMsg::GetInstance().get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);
            if (ctx->getSysNonblock()) {
                // arg |= O_NONBLOCK;           // init()中已经设置了
            } else {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
        case F_GETFL: {
            int arg = fcntl_f(fd, cmd);
            va_end(va);
            auto ctx = flexy::FdMsg::GetInstance().get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return arg;
            }
            if (ctx->getUserNonblock()) {
                return arg | O_NONBLOCK;
            } else {
                return arg & ~O_NONBLOCK;
            }
        }
        case F_DUPFD:
            break;
        case F_DUPFD_CLOEXEC:
            break;
        case F_SETFD:
            break;
        case F_SETOWN:
            break;
        case F_SETSIG:
            break;
        case F_SETLEASE:
            break;
        case F_NOTIFY:
            break;
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        case F_GETFD:
            break;
        case F_GETOWN:
            break;
        case F_GETSIG:
            break;
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return fcntl_f(fd, cmd);    
        }
        case F_SETLK:
            break;
        case F_SETLKW:
            break;
        case F_GETLK: {
            struct flock* arg = va_arg(va, struct flock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        case F_GETOWN_EX:
            break;
        case F_SETOWN_EX: {
            struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        default: {
            va_end(va);
            return fcntl_f(fd, cmd);
        }
    }
    va_end(va);
    return fcntl_f(fd, cmd);
}

int ioctl(int fd, unsigned long request, ...) {
    if (!flexy::t_hook_enable) {
        if (!ioctl_f) {
            flexy::hook_init();
        }
    }
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if (request == FIONBIO) {
        bool user_nonblock = !!*(int*)arg;
        auto ctx = flexy::FdMsg::GetInstance().get(fd);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    
    return ioctl_f(fd, request, arg);
} 

int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
    if (!getsockopt_f) {
        flexy::hook_init();
    }
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
    if (!flexy::t_hook_enable) {
        if (!setsockopt_f) {
            flexy::hook_init();
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if (level == SOL_SOCKET) {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            auto ctx = flexy::FdMsg::GetInstance().get(sockfd);
            if (ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
        
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

} // extern "C"

