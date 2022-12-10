#include <flexy/flexy.h>

using namespace flexy;

static auto&& g_logger = FLEXY_LOG_ROOT();

void run() {
    auto addr = Address::LookupAnyIPAddress("0.0.0.0:8050");
    auto sock = Socket::CreateUDP(addr->getFamily());
    if (sock->bind(addr)) {
        FLEXY_LOG_INFO(g_logger) << "udp bind : " << *addr;
    } else {
        FLEXY_LOG_ERROR(g_logger) << "udp bind : " << *addr << " fail";
        return;
    }
    while (true) {
        char buff[1024];
        Address::ptr from = std::make_shared<IPv4Address>();
        int len = sock->recvFrom(buff, 1024, from);
        if (len > 0) {
            buff[len] = 0;
            FLEXY_LOG_INFO(g_logger) << "recv: " << buff << " from: " << *from;
            len = sock->sendTo(buff, len, from);
            if (len < 0) {
                FLEXY_LOG_INFO(g_logger) << "send: " << buff << " to: " << *from
                                         << " error = " << len;
            }
        }
    }
}

int main() {
    IOManager iom;
    go run;
}