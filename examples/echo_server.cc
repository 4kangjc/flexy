#include <flexy/flexy.h>

using namespace flexy;

static auto&& g_logger = FLEXY_LOG_ROOT();

class EchoServer : public TcpServer {
public:
};

void run() {
    auto es = std::make_shared<EchoServer>();
    es->onHandleClient([](const TcpServer::ptr& self, const Socket::ptr& client) {
        FLEXY_LOG_INFO(g_logger) << "handleClient " << *client;
        while (true) {
            std::string s;
            s.resize(4096);
            if (client->recv(s) <= 0)  break;
            client->send(s);
        }
    });
    auto addr = Address::LookupAny("0.0.0.0:8021");
    es->bind(addr);
    es->start();
}

int main() {
    IOManager iom(2, true, "echo");
    iom.async(run); 
}