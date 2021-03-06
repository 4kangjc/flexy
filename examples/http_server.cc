#include <flexy/flexy.h>

using namespace flexy;

static auto&& g_logger = FLEXY_LOG_ROOT();

void run() {
    auto addr = Address::LookupAnyIPAddress("[::]:8020", AF_INET6);
    // auto addr = Address::LookupAnyIPAddress("0.0.0.0:8020");
    if (!addr) {
        FLEXY_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    auto http_server = std::make_shared<http::HttpServer>(true);

    while (!http_server->bind(addr)) {}
    http_server->start();
}

int main(int argc, char** argv) {
    g_logger->setLevel(LogLevel::INFO);
    int count = 1;
    if (argc > 1) {
        count = atoi(argv[1]);
    }
    IOManager worker(count, true, "worker");
    go run;
}
