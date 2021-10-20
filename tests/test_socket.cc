#include <flexy/net/socket.h>
#include <flexy/schedule/iomanager.h>
#include <flexy/util/log.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test_socket() {
    auto addr = flexy::Address::LookupAnyIPAddress("www.baidu.com", AF_UNSPEC);
    //auto addr = flexy::Address::LookupAnyIpAddress("ipv6.test-ipv6.com", AF_UNSPEC);
    if (addr) {
        FLEXY_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        FLEXY_LOG_ERROR(g_logger) << "get address fail";
        return;
    }
    auto sock = flexy::Socket::CreateTCP(addr->getFamily());
    addr->setPort(80);
    if (!sock->connect(addr)) {
        FLEXY_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        FLEXY_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }
    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff);
    if (rt <= 0) {
        FLEXY_LOG_ERROR(g_logger) << "send fail, rt = " << rt;
        return;
    } 
    std::string buf;
    buf.resize(4096);
    rt = sock->recv(buf);
    if (rt <= 0) {
        FLEXY_LOG_ERROR(g_logger) << "recv fail, rt = " << rt;
        return;
    }
    buf.resize(rt);
    FLEXY_LOG_INFO(g_logger) << buf; 
}

int main() {
    flexy::IOManager iom;
    iom.async(test_socket);

    return 0;
}