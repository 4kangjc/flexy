#include "address.h"
#include "edian.h"
#include "flexy/util/log.h"
#include "flexy/util/util.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

template <typename T>
static T CreateMask(uint32_t bits) {

}

template <typename T>
static uint32_t CountBytes(T value) {
    uint32_t res = 0;
    while (value > 0) {
        value &= value - 1; 
        ++res;
    }
    return res;
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
    if (!addr) {
        return nullptr;
    }
    Address::ptr result;
    switch (addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnkownAddress(*addr));
            break;
    }
    return result;
}

std::optional<std::vector<Address::ptr>>
Address::Lookup(std::string_view host, int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    addrinfo hints, *res, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;

    std::string ip;                             // ipv4 or ipv6 or 域名
    const char* service = nullptr;              // port

    if (!host.empty() && host[0] == '[') {
        const char* endipv6 = (const char*)memchr(host.data() + 1, ']', host.size() - 1);
        if (endipv6) {
            if (*(endipv6 + 1) == ':') {
                service = endipv6 + 2;
            }
            ip = std::string(&host[1], endipv6 - host.data() - 1);
        }
    }

    if (ip.empty()) {
        service = (const char*)memchr(host.data(), ':', + host.size());
        if (service) {
            if (!memchr(service + 1, ':', host.data() + host.size() - service - 1)) {
                ip = std::string(host.data(), service - host.data());
                ++service;
            }
        } 
    }

    if (ip.empty()) {
        ip = host;
    }
    int rt = getaddrinfo(ip.data(), service, &hints, &res);
    if (rt) {
        FLEXY_LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", " << family 
            << ", " << type << ") err = " << rt << " errstr = " << strerror(rt);
        return {};
    }

    next = res;
    while (next) {
        result.push_back(Create(next->ai_addr, next->ai_addrlen));
        next = next->ai_next;
    }
    freeaddrinfo(res);
    if (result.empty()) {
        return {};
    } 
    return result;
}

Address::ptr Address::LookupAny(std::string_view host, int family, int type, int protocol) {
    auto&& result = Lookup(host, family, type, protocol);
    if (result) {
        return (*result)[0];
    }
    return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(std::string_view host, int family,
                                            int type, int protocol) {
    auto&& result = Lookup(host, family, type, protocol);
    if (result) {
        for (auto& i : *result) {
            auto v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) {
                return v;
            }
        }
    }
    return nullptr;
}

std::optional<std::multimap<std::string, std::pair<Address::ptr, uint32_t>>>
Address::GetInterfaceAddress(int family) {
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> result;
    ifaddrs *next, *res;
    if (getifaddrs(&res) != 0) {
        FLEXY_LOG_FMT_ERROR(g_logger, "Address::GetInterfaceAddress getifaddrs error = {}"
        ", errstr = {}", errno, strerror(errno));
        return {};
    }
    try {
        for (next = res; next != nullptr; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            switch (next->ifa_addr->sa_family) {
                case AF_INET: {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                    uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                    prefix_len = CountBytes(netmask);
                    break;
                }
                case AF_INET6: {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                    prefix_len = 0;
                    for (int i = 0; i < 16; ++i) {
                        prefix_len += CountBytes(netmask.s6_addr[i]);
                    }
                    break;
                }
                default:    
                    break;
            }
            if (addr) {
                result.emplace(next->ifa_name, std::make_pair(addr, prefix_len));
            }
        }
    } catch (...) {
        FLEXY_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress exception";
        freeifaddrs(res);
        return {};
    }
    freeifaddrs(res);
    return result;
}

std::optional<std::vector<std::pair<Address::ptr, uint32_t>>> 
Address::GetInterfaceAddress(const std::string& iface, int family) {
    std::vector<std::pair<Address::ptr, uint32_t>> result;
    if (iface.empty() || iface == "*") {
        if (family == AF_INET || family == AF_UNSPEC) {
            result.emplace_back(new IPv4Address(), 0u);
        }
        if (family == AF_INET6 || family == AF_UNSPEC) {
            result.emplace_back(new IPv6Address(), 0u);
        }
        return result;
    }
    auto res = GetInterfaceAddress(family);
    if (!res) {
        return {};
    }
    auto its = res->equal_range(iface);
    for (auto it = its.first; it != its.second; ++it) {
        result.emplace_back(it->second);
    }
    if (result.empty()) {
        return {};
    }
    return result;
}

inline int Address::getFamily() const {
    return getAddr()->sa_family;
}

std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address &rhs) const {
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if (result < 0) {
        return true;
    } else if (result > 0) {
        return false;
    } else if (getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}

inline bool Address::operator==(const Address &rhs) const {
    return getAddrLen() == rhs.getAddrLen() 
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen());
}

inline bool Address::operator!=(const Address &rhs) const {
    return !(*this == rhs);
}

IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
    addrinfo hints, *res;
    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;            // 以数字格返回主机地址
    hints.ai_family = AF_UNSPEC;

    int rt = getaddrinfo(address, nullptr, &hints, &res);
    if (rt) {
        FLEXY_LOG_FMT_ERROR(g_logger, "IPAddress::Create({}, {}), rt = "
        "{}, errno = {}, errstr = {}", address, port, rt, errno, strerror(errno));
        return nullptr;
    }

    try {
        auto result = std::dynamic_pointer_cast<IPAddress>(Address::Create(res->ai_addr, res->ai_flags));
        if (result) {
            result->setPort(port);
        }
        freeaddrinfo(res);
        return result;
    } catch (...) {
        FLEXY_LOG_ERROR(g_logger) << "IPAddress::Create catch exception";
        freeaddrinfo(res);
        return nullptr;
    }
}

IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port) {
    auto rt = std::make_shared<IPv4Address>(INADDR_ANY, port);
    int result = inet_pton(AF_INET, address, &rt->addr_.sin_addr);
    if (result <= 0) {
        FLEXY_LOG_FMT_ERROR(g_logger, "IPv4Address::Create({}, {}), "
        "rt = {} errno = {}, errstr = {}", address, port, result,
        errno, strerror(errno));
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    bzero(&addr_, sizeof(addr_));
    addr_.sin_addr.s_addr = byteswap(address);
    addr_.sin_port = byteswap(port);
    addr_.sin_family = AF_INET;
}

inline const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&addr_;
}

inline sockaddr* IPv4Address::getAddr() {
    return (sockaddr*)&addr_; 
}

inline socklen_t IPv4Address::getAddrLen() const {
    return sizeof(addr_);
}

std::ostream& IPv4Address::insert(std::ostream& os) const {
    uint32_t addr = addr_.sin_addr.s_addr;
    os << format("%d.%d.%d.%d:%d", (addr >> 0) & 0xff, (addr >> 8) & 0xff, 
        (addr >> 16) & 0xff, (addr >> 24) & 0xff, byteswap(addr_.sin_port));
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    return nullptr;
}

IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
    return nullptr;
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    return nullptr;
}

uint32_t IPv4Address::getPort() const {
    return byteswap(addr_.sin_port);
}

void IPv4Address::setPort(uint16_t v) {
    addr_.sin_port = byteswap(v);
}

IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
    auto rt = std::make_shared<IPv6Address>();
    rt->addr_.sin6_port = byteswap(port);
    int result = inet_pton(AF_INET6, address, &rt->addr_.sin6_addr);
    if (result <= 0) {
        FLEXY_LOG_FMT_ERROR(g_logger, "IPv6Address::Create({}, {}), rt = {}"
        ", errno = {}, errstr = {}", address, port, result, errno, strerror(errno));
    }
    return rt; 
}

IPv6Address::IPv6Address() {
    bzero(&addr_, sizeof(addr_));
    addr_.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) : IPv6Address() {
    addr_.sin6_port = byteswap(port);
    memcpy(&addr_.sin6_addr.s6_addr, address, 16);
}

inline const sockaddr* IPv6Address::getAddr() const {
    return (sockaddr*)&addr_;
}

inline sockaddr* IPv6Address::getAddr() {
    return (sockaddr*)&addr_;
}

inline socklen_t IPv6Address::getAddrLen() const {
    return sizeof(addr_);
}

std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "[";
    uint16_t* addr = (uint16_t*)&addr_.sin6_addr.s6_addr;
    os << std::hex;
    bool use_zero = false;
    for (size_t i = 0; i < 8; ++i) {
        if (addr[i] == 0 && !use_zero) {
            continue;
        }
        if (i && !addr[i - 1] && !use_zero) {
            os << ":";
            use_zero = true;
        }
        if (i) {
            os << ":";
        }
        os << byteswap(addr[i]);
    }
    if (!use_zero && !addr[7]) {
        os << "::";
    }
    os << std::dec << "]:" << byteswap(addr_.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    return nullptr;
}

IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
    return nullptr;
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    return nullptr;
}

inline uint32_t IPv6Address::getPort() const {
    return byteswap(addr_.sin6_port);
}

void IPv6Address::setPort(uint16_t v) {
    addr_.sin6_port = byteswap(v);
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
    bzero(&addr_, sizeof(addr_));
    addr_.sun_family = AF_UNIX;
    length_ = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(std::string_view path) {
    bzero(&addr_, sizeof(addr_));
    addr_.sun_family = AF_UNIX;
    length_ = path.size() + 1;
    if (!path.empty() && path[0] == '\0') {
        --length_;
    }
    memcpy(addr_.sun_path, path.data(), length_);
    length_ += offsetof(sockaddr_un, sun_path);
}

const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&addr_;
}

sockaddr* UnixAddress::getAddr()  {
    return (sockaddr*)&addr_;
}

socklen_t UnixAddress::getAddrLen() const {
    return length_;
}

void UnixAddress::setAddrLen(uint32_t v) {
    length_ = v;
}

std::ostream& UnixAddress::insert(std::ostream& os) const {
    if (length_ > offsetof(sockaddr_un, sun_path) && addr_.sun_path[0] == '\0') {
        return os << "\\0" << std::string_view(addr_.sun_path + 1, 
                    length_ - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << addr_.sun_path;
}

UnkownAddress::UnkownAddress(int family) {
    bzero(&addr_, sizeof(addr_));
    addr_.sa_family = family;
}

const sockaddr* UnkownAddress::getAddr() const {
    return &addr_;
}

sockaddr* UnkownAddress::getAddr() {
    return &addr_;
}

socklen_t UnkownAddress::getAddrLen() const {
    return sizeof(addr_);
}

std::ostream& UnkownAddress::insert(std::ostream& os) const {
    os << "[UnkownAddress family = " << addr_.sa_family << "]";
    return os;
}

} // namespace flexy