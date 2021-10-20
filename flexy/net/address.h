#pragma once

#include <optional>
#include <memory>
#include <netinet/in.h>
#include <vector>
#include <string_view>
#include <map>
#include <sys/un.h>

namespace flexy {

class IPAddress;
class Address {
public:
    using ptr = std::shared_ptr<Address>;
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);
    static std::optional<std::vector<Address::ptr>> Lookup(std::string_view host, int family = AF_INET,
                                                           int type = 0, int protocol = 0);
    static Address::ptr LookupAny(std::string_view host, int family = AF_INET, int type = 0, int protocol = 0);
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(std::string_view host, int family = AF_INET,
                                                         int type = 0, int protocol = 0);
    static std::optional<std::multimap<std::string, std::pair<Address::ptr, uint32_t>>>
    GetInterfaceAddress(int family = AF_INET);
    static std::optional<std::vector<std::pair<Address::ptr, uint32_t>>> GetInterfaceAddress(
            const std::string& iface, int family = AF_INET);
    virtual ~Address() = default;
    int getFamily() const;
    virtual const sockaddr* getAddr() const = 0;
    virtual sockaddr* getAddr() = 0;
    virtual socklen_t getAddrLen() const = 0;

    virtual std::ostream& insert(std::ostream& os) const { return os; }
    std::string toString() const;

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};

class IPAddress : public Address {
public:
    using ptr = std::shared_ptr<IPAddress>;
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;
};

class IPv4Address : public IPAddress {
public:
    using ptr = std::shared_ptr<IPv4Address>;
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
    IPv4Address(const sockaddr_in& rhs) : addr_(rhs) {}
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override; 

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;   
private:
    sockaddr_in addr_;
};

class IPv6Address : public IPAddress {
public:
    using ptr = std::shared_ptr<IPv6Address>;
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);
    IPv6Address();
    IPv6Address(const sockaddr_in6& rhs) : addr_(rhs) {}
    IPv6Address(const uint8_t address[16], uint16_t port = 0);
    
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override; 

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in6 addr_;
};

class UnixAddress : public Address {
public:
    UnixAddress();
    UnixAddress(std::string_view path);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    void setAddrLen(uint32_t v);
private:
    sockaddr_un addr_;
    socklen_t length_;
};

class UnkownAddress : public Address {
public:
    UnkownAddress(const sockaddr& addr) : addr_(addr) {}
    UnkownAddress(int family);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr addr_;
};

inline std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.insert(os);
}

} // namespace flexy