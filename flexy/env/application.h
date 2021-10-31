#pragma once

#include "flexy/http/http_server.h"

namespace flexy {

class Application {
public:
    Application();
    bool init(int argc, char** argv);
    bool run();
private:
    int main(int argc, char** argv);
    void run_fiber();
private:
    int argc_ = 0;
    char** argv_ = nullptr;
    std::vector<http::HttpServer::ptr> httpservers_;
    inline static Application* s_instance = nullptr;
};

} // namespace flexy