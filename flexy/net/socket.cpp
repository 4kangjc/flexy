#include "socket.h"
#include "flexy/schedule/iomanager.h"
#include "flexy/util/macro.h"
#include "fd_manager.h"
#include "hook.h"

#include <string>
#include <netinet/tcp.h>

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

Socket::ptr Socket::CreateTCP(int family) {
    auto sock = std::make_shared<Socket>(family, TCP);
    return sock;
}

Socket::ptr Socket::CreateUDP(int family) {
    auto sock = std::make_shared<Socket>(family, UDP);
    sock->newSock();
    sock->isConnected_ = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
    auto sock = std::make_shared<Socket>(IPv4, TCP);
    return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
    auto sock = std::make_shared<Socket>(IPv4, UDP);
    sock->newSock();
    sock->isConnected_ = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
    auto sock = std::make_shared<Socket>(IPv6, TCP);
    return sock;
}

Socket::ptr Socket::CreateUDPSocket6() {
    auto sock = std::make_shared<Socket>(IPv6, UDP);
    sock->newSock();
    sock->isConnected_ = true;
    return sock;
}

Socket::Socket(int family, int type, int protocol) 
    : sock_(-1), family_(family), type_(type), 
      protocol_(protocol), isConnected_(false) {
}

Socket::~Socket() {
    close();
}

int64_t Socket::getSendTimeout() {
    auto ctx = FdMsg::GetInstance().get(sock_);
    if (ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

void Socket::setSendTimeout(int64_t v) {
    timeval tv{(int)v / 1000, (int)(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout() {
    auto ctx = FdMsg::GetInstance().get(sock_);
    if (ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t v) {
    timeval tv{(int)v / 1000, (int)(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}


bool Socket::getOption(int level, int option, void* result, socklen_t* len) {
    int rt = getsockopt(sock_, level, option, result, len);
    if (rt) {
        FLEXY_LOG_DEBUG(g_logger) << "getOption sock = " << sock_ << " level = "
        << level << " option = " << option << " errno = " << errno << " errstr = " 
        << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const void* result, socklen_t len) {
    int rt = setsockopt(sock_, level, option, result, len);
    if (rt) {
        FLEXY_LOG_DEBUG(g_logger) << "setOption sock = " << sock_ << " level = "
        << level << " option = " << option << " errno = " << errno << " errstr = " 
        << strerror(errno);
        return false;
    }
    return true;
}

Socket::ptr Socket::accept() {
    int newsock = ::accept(sock_, nullptr, nullptr);
    if (newsock == -1) {
        FLEXY_LOG_ERROR(g_logger) << "accept(" << sock_ << ") errno = " << errno
        << " errstr = " << strerror(errno);
        return nullptr;
    }
    auto sock = std::make_shared<Socket>(family_, type_, protocol_);
    if (sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

bool Socket::bind(const Address::ptr& addr) {
    if (sock_ == -1) {
        newSock();
        if (FLEXY_UNLIKELY(sock_ == -1)) {
            return false;
        }
    }
    if (FLEXY_UNLIKELY(addr->getFamily() != family_)) {
        FLEXY_LOG_FMT_ERROR(g_logger, "bind sock.family = {}, addr.family = {}"
        " not equal, addr = ", family_, addr->getFamily(), addr->toString());
        return false;
    }
    if (::bind(sock_, addr->getAddr(), addr->getAddrLen())) {
        FLEXY_LOG_ERROR(g_logger) << "bind error = " << errno << ", errstr = "
        << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

bool Socket::reconnect(uint64_t timeout_ms) {
    if (!remoteAddress_) {
        FLEXY_LOG_ERROR(g_logger) << "reconnect remoteAddress is null";
        return false;
    }
    localAddress_ = nullptr;
    return connect(remoteAddress_, timeout_ms);
}

bool Socket::connect(const Address::ptr& addr, uint64_t timeout_ms) {
    if (sock_ == -1) {
        newSock();
        if (FLEXY_UNLIKELY(sock_ == -1)) {
            return false;
        }
    }

    if (FLEXY_UNLIKELY(addr->getFamily() != family_)) {
        FLEXY_LOG_FMT_ERROR(g_logger, "bind sock.family = {}, addr.family = {}"
        " not equal, addr = ", family_, addr->getFamily(), addr->toString());
        return false;
    }

    if (timeout_ms == ~0ull) {
        if (::connect(sock_, addr->getAddr(), addr->getAddrLen())) {
            FLEXY_LOG_ERROR(g_logger) << "sock = " << sock_ << " connect(" 
            << addr->toString() << ") errno = " << errno << " errstr = " 
            << strerror(errno);
            close();
            return false;
        }
    } else {
        if (::connect_with_timeout(sock_, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            FLEXY_LOG_ERROR(g_logger) << "sock = " << sock_ << " connect(" 
            << addr->toString() << ") errno = " << errno << " errstr = " 
            << strerror(errno);
            close();
            return false;
        }
    }
    isConnected_ = true;
    getLocalAddress();
    getRemoteAddress();
    return true;
}

bool Socket::listen(int backlog) {
    if (sock_ == -1) {
        FLEXY_LOG_ERROR(g_logger) << "listen error sock = -1";
        return false;
    }
    if (::listen(sock_, backlog)) {
        FLEXY_LOG_ERROR(g_logger) << "listen error, errno = " << errno << ", errstr"
        " = " << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::close() {
    if (!isConnected_ && sock_ == -1) {
        return true;
    }
    isConnected_ = false;
    if (sock_ != -1) {
        ::close(sock_);
        sock_ = -1;
    }
    return false;
}

int Socket::send(const void* buffer, size_t length, int flags) {
    if (isConnected_) {
        return ::send(sock_, buffer, length, flags);
    }
    return -1;
}

int Socket::send(std::string_view s, int flags) {
    int rt = send(s.data(), s.size(), flags);
    if (rt < 0) {
        FLEXY_LOG_INFO(g_logger) << "sock = " << sock_ << " send error,"
        " errrno = " << errno << ", errstr = " << strerror(errno);
    }
    return rt;
}

int Socket::send(const iovec* buffer, size_t length, int flags) {
    if (isConnected_) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        return ::sendmsg(sock_, &msg, flags);
    }
    return -1;
}
 
int Socket::sendTo(const void* buffer, size_t length, const Address::ptr& to, int flags) {
    if (isConnected_) {
        return ::sendto(sock_, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

int Socket::sendTo(std::string_view s, const Address::ptr& to, int flags) {
    return sendTo(s.data(), s.size(), to, flags);
}

int Socket::sendTo(const iovec* buffer,  size_t length, const Address::ptr& to, int flags) {
    if (isConnected_) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(sock_, &msg, flags);
    }
    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags) {
    if (isConnected_) {
        return ::recv(sock_, buffer, length, flags);
    }
    return -1;
}

int Socket::recv(std::string& s, int flags) {
    int rt = recv(s.data(), s.size(), flags);
    if (rt < 0) {
        FLEXY_LOG_INFO(g_logger) << "sock = " << sock_ << " recv error,"
        " errrno = " << errno << ", errstr = " << strerror(errno);
    } else if (rt == 0) {
        FLEXY_LOG_DEBUG(g_logger) << "sock = " << sock_ << " close";
    }
    return rt;
}

int Socket::recv(iovec* buffer, size_t length, int flags) {
    if (isConnected_) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buffer;
        msg.msg_iovlen = length;
        return ::recvmsg(sock_, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr& from, int flags) {
    if (isConnected_) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(sock_, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

int Socket::recvFrom(std::string& s, Address::ptr& from, int flags) {
    return recvFrom(s.data(), s.size(), from, flags);
}

int Socket::recvFrom(iovec* buffer, size_t length, Address::ptr& from, int flags) {
    if (isConnected_) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buffer;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(sock_, &msg, flags);
    }
    return 0;
}


Address::ptr Socket::getRemoteAddress() {
    if (remoteAddress_) {
        return remoteAddress_;
    }
    Address::ptr result;
    switch (family_) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnkownAddress(family_));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if (getpeername(sock_, result->getAddr(), &addrlen)) {
        FLEXY_LOG_ERROR(g_logger) << "getpeername error sock = " << sock_
        << " errno = " << errno << " errstr = " << strerror(errno);
        return std::make_shared<UnkownAddress>(family_);
    }
    if (family_ == AF_UNIX) {
        auto addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    remoteAddress_ = result;
    
    return result;
}

Address::ptr Socket::getLocalAddress() {
    if (localAddress_) {
        return localAddress_;
    }
    Address::ptr result;
    switch (family_) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnkownAddress(family_));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if (getsockname(sock_, result->getAddr(), &addrlen)) {
        FLEXY_LOG_ERROR(g_logger) << "getpeername error sock = " << sock_
        << " errno = " << errno << " errstr = " << strerror(errno);
        return std::make_shared<UnkownAddress>(family_);
    }
    if (family_ == AF_UNIX) {
        auto addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    localAddress_ = result;

    return result;
}

int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const {
    os << "[Socket sock = " << sock_ << " is_connected = " 
    << isConnected_ << " family = " << (Family)family_ 
    << " type = " << (Type)type_ << " protocol = " << protocol_;
    if (localAddress_) {
        os << " local_address = " << localAddress_->toString();
    }
    if (remoteAddress_) {
        os << " remote_address = " << remoteAddress_->toString();
    }
    return os << "]";
}

std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelRead(sock_);
}

bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelWrite(sock_);
}

bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelRead(sock_);
}

bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAll(sock_);
}

void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if (type_ == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);           // 禁用了Nagle算法，允许小包的发送
    }
}   

void Socket::newSock() {
    sock_ = socket(family_, type_, protocol_);
    if (FLEXY_LIKELY(sock_ != -1)) {
        initSock();
    } else {
        FLEXY_LOG_FMT_ERROR(g_logger, "socket({}, {}, {}), errno = {},"
        " errstr = {}", family_, type_, protocol_, errno, strerror(errno));
    }
}

bool Socket::init(int sock) {
    auto ctx = FdMsg::GetInstance().get(sock);
    if (ctx && ctx->isSocket() && !ctx->isClose()) {
        sock_ = sock;
        isConnected_ = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

} // namespace flexy