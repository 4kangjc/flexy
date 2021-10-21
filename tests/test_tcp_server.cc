#include <flexy/net/tcp_server.h>
#include <flexy/util/log.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void run() {
    auto addr = flexy::Address::LookupAny("0.0.0.0:8013");
    auto tcp_server = std::make_shared<flexy::TcpServer>();
    while (!tcp_server->bind(addr)) {
        // sleep(2);
    }
    tcp_server->start();
}

int main() {
    flexy::IOManager iom(2);
    iom.async(run);
}