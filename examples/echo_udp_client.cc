#include <flexy/flexy.h>

using namespace flexy;

static auto&& g_logger = FLEXY_LOG_ROOT();

const char* ip;

void run() {
    auto addr = Address::LookupAnyIPAddress(ip);
    if (!addr) {
        FLEXY_LOG_ERROR(g_logger) << "invalid ip: " << ip;
        return;
    }
    auto sock = Socket::CreateUDP(addr->getFamily());
    go [sock]() {
        Address::ptr addr = std::make_shared<IPv4Address>();
        FLEXY_LOG_INFO(g_logger) << "begin recv";
        while (true) {
            char buff[1024];
            int len = sock->recvFrom(buff, 1024, addr);
            if (len > 0) {
                std::cout << std::endl << " recv: " << std::string_view(buff, len)
                << " from: " << *addr << std::endl;
            }
        }
    };
    
    // sleep(1);
    while (true) {
        std::string line;
        std::cout << "input> ";
        // std::getline(std::cin, line);
        getline(async_cin, line);
        if (!line.empty()) {
            int len = sock->sendTo(line.c_str(), line.size(), addr);
            if (len < 0) {
                int err = sock->getError();
                FLEXY_LOG_ERROR(g_logger) << "send error err = " << err
                << " errstr = " << strerror(err) << " len = " << len
                << " addr = " << *addr << " sock = " << sock;
            } else {
                FLEXY_LOG_INFO(g_logger) << "send " << line << " len = " << len;
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return 0;
    }
    std::string s = argv[1] + std::string(":") + argv[2];
    ip = s.c_str();
    IOManager iom;
    go run;
}