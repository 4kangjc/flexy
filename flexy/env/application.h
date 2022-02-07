#pragma once

#include "flexy/http/http_server.h"
#include "flexy/http/ws_server.h"
namespace flexy {

class Application {
public:
    Application();
    bool init(int argc, char** argv);
    bool run();

    template <class T>
    std::vector<T> getServer() const;

    auto listAllServer() const { return servers_; }
private:
    int main(int argc, char** argv);
    void run_fiber();
private:
    int argc_ = 0;
    char** argv_ = nullptr;
    // std::vector<http::HttpServer::ptr> httpservers_;
    std::unordered_map<std::string, std::vector<TcpServer::ptr>> servers_;
    inline static Application* s_instance = nullptr;
};

template <class T>
std::vector<T> Application::getServer() const {
#define XX(type) \
    if (auto it = servers_.find(#type); it != servers_.end()) {      \
        std::vector<T> servers;                                      \
        servers.reserve(it->second.size());                          \
        for (auto& server : it->second) {                            \
            servers.push_back(std::dynamic_pointer_cast<T>(server)); \
        }                                                            \
        return servers;                                              \
    }                                                                \
    return {}

    if constexpr (std::is_same_v<T, http::HttpServer>) {
        XX(http);
    } else if constexpr (std::is_same_v<T, http::WSServer>) {
        XX(ws);
    } else if constexpr (std::is_same_v<T, TcpServer>) {
        XX(tcp);
    } else {
        return {};
    }
#undef XX
}

} // namespace flexy