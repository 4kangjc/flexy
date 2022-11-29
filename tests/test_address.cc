#include <flexy/net/address.h>
#include <flexy/util/macro.h>

static auto g_logger = FLEXY_LOG_ROOT();

void testIPv6() {
    auto addr = flexy::IPv6Address::Create("2001:0db8:85a3::8a2e:0370:7334", 12345);
    FLEXY_LOG_INFO(g_logger) << "IPAddress        = " << addr->toString();
    FLEXY_LOG_INFO(g_logger) << "broadcastAddress = " << addr->broadcastAddress(44)->toString();
    FLEXY_LOG_INFO(g_logger) << "networkAddress   = " << addr->networkAddress(44)->toString();
    FLEXY_LOG_INFO(g_logger) << "subnetMask       = " << addr->subnetMask(44)->toString();
}

void testUnixAddress() {
    flexy::UnixAddress ua;
    FLEXY_LOG_INFO(g_logger) << ua.getAddrLen();
}

void testIpv4() {
    auto addr = flexy::IPv4Address::Create("172.17.237.213", 23);
    if (addr) {
        FLEXY_LOG_INFO(g_logger) << "IPAddress        = " << addr->toString();
        FLEXY_LOG_INFO(g_logger) << "broadcastAddress = " << addr->broadcastAddress(20)->toString();
        FLEXY_LOG_INFO(g_logger) << "networkAddress   = " << addr->networkAddress(20)->toString();
        FLEXY_LOG_INFO(g_logger) << "subnetMask       = " << addr->subnetMask(20)->toString();
    }
    // auto baidu = flexy::IPAddress::Create("www.baidu.com");
    auto baidu = flexy::IPAddress::Create("103.235.46.39", 80);
    if (baidu) { 
        FLEXY_LOG_INFO(g_logger) << baidu->toString();
    }
}

void test() {
    auto addpsptr = flexy::Address::Lookup("www.baidu.com:ftp");
    FLEXY_ASSERT(addpsptr);
    for (auto& addr : *addpsptr) {
        FLEXY_LOG_INFO(g_logger) << "www.baidu.com ==> " << addr->toString();
    }
}

void test_iface() {
    auto opt = flexy::Address::GetInterfaceAddress();
    FLEXY_ASSERT(opt);
    bool flag = false;
    const char* sv = nullptr;
    for (auto& [x, y] : *opt) {
        if (!flag) {
            sv = x.c_str();
            flag = true;
        }
        FLEXY_LOG_INFO(g_logger) << x << " - " << y.first->toString() << " - " << y.second;
    }
    auto wlan0 = flexy::Address::GetInterfaceAddress(sv);
    if (!wlan0) {
        FLEXY_LOG_INFO(g_logger) << "No such name as " << sv;
    }
    for (auto& [x, y] : *wlan0) {
        FLEXY_LOG_INFO(g_logger) << x->toString() << " - " << y;
    }
}

int main() {
    testIPv6();
    //testUnixAddress();
    FLEXY_LOG_DEBUG(g_logger) << "--------------------------------------------------------";
    testIpv4();
    FLEXY_LOG_DEBUG(g_logger) << "---------------------------------------------------------";
    test();
    FLEXY_LOG_DEBUG(g_logger) << "---------------------------------------------------------";
    test_iface();
}