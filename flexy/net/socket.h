#pragma once

#include "address.h"
#include "flexy/util/noncopyable.h"
#include <memory>
#include <ostream>

namespace flexy {

class Socket : public std::enable_shared_from_this<Socket>, noncopyable {
public:
    using ptr = std::shared_ptr<Socket>;

    enum Type {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    enum Family {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        Unix = AF_UNIX
    };

    static Socket::ptr CreateTCP(int family);
    static Socket::ptr CreateUDP(int family);

    static Socket::ptr CreateTCPSocket();
    static Socket::ptr CreateUDPSocket();

    static Socket::ptr CreateTCPSocket6();
    static Socket::ptr CreateUDPSocket6();

    Socket(int family, int type, int protocol = 0);
    ~Socket();

    int64_t getSendTimeout();
    void setSendTimeout(int64_t v);

    int64_t getRecvTimeout();
    void setRecvTimeout(int64_t v);

    bool getOption(int level, int option, void* result, socklen_t* len);

    template <typename T>
    bool getOption(int level, int option, T& result) {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    bool setOption(int level, int option, const void* result, socklen_t len);

    template <typename T>
    bool setOption(int level, int option, T& result) {
        return setOption(level, option, &result, sizeof(T));
    }

    Socket::ptr accept();

    bool bind(const Address::ptr& addr);
    bool connect(const Address::ptr& addr, uint64_t timeout_ms = -1);
    bool listen(int backlog = SOMAXCONN);
    bool close();

    int send(const void* buffer, size_t length, int flags = 0);
    int send(std::string_view s, int flags = 0);
    int send(const iovec* buffer, size_t length, int flags = 0);
    int sendTo(const void* buffer, size_t length, const Address::ptr& to, int flags = 0);
    int sendTo(std::string_view s, const Address::ptr& to, int flags = 0);
    int sendTo(const iovec* buffer,  size_t length, const Address::ptr& to, int flags = 0);

    int recv(void* buffer, size_t length, int flags = 0);
    int recv(std::string& s, int flags = 0);
    int recv(iovec* buffer, size_t length, int flags = 0);
    int recvFrom(void* buffer, size_t length, Address::ptr& from, int flags = 0);
    int recvFrom(std::string& s, Address::ptr& from, int flags = 0);
    int recvFrom(iovec* buffer, size_t length, Address::ptr& from, int flags = 0);

    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();

    int getFamily() const { return family_; }
    int getType() const { return type_; }
    int getProtocol() const { return protocol_; }

    bool isConnected() const { return isConnected_; }
    bool isValid() const { return sock_ != -1; }
    int getError();

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;
    int getSocket() const { return sock_; }

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();
private:
    void initSock();
    void newSock();
    bool init(int sock);
private:
    int sock_;
    int family_;
    int type_;
    int protocol_;
    bool isConnected_;

    Address::ptr localAddress_;
    Address::ptr remoteAddress_;
};

inline std::ostream& operator<<(std::ostream& os, const Socket& sock) {
    return sock.dump(os);
}

inline std::ostream& operator<<(std::ostream& os, Socket::Type type) {
    if (type == Socket::Type::TCP) {
        os << "TCP";
    } else if (type == Socket::Type::UDP) {
        os << "UDP";
    } else {
        os << "UNKOWN Type";
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, Socket::Family family) {
    if (family == AF_INET) {
        os << "IPv4";
    } else if (family == AF_INET6) {
        os << "IPv6";
    } else {
        os << "Unix";
    }
    return os;
}
    
} // namespace flexy
