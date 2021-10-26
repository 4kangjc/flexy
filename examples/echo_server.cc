#include <flexy/flexy.h>

using namespace flexy;

static auto&& g_logger = FLEXY_LOG_ROOT();

class EchoServer : public TcpServer {
public:
    void handleClient(const Socket::ptr& client) override {
        FLEXY_LOG_DEBUG(g_logger) << "handleClient " << *client;
        std::string s;
        s.resize(4096);
        while (true) {
            memset(s.data(), 0, 4096);
            if (client->recv(s) <= 0)  break;
            client->send(s);
        }
    }
};

void run() {
    auto es = std::make_shared<EchoServer>();
    auto addr = Address::LookupAny("0.0.0.0:8021");
    es->bind(addr);
    es->start();
}

int main() {
    IOManager iom(2, true, "echo");
    iom.async(run); 
}