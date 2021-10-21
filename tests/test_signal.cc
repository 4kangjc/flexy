#include <flexy/env/signal.h>
#include <flexy/util/log.h>
#include <flexy/schedule/iomanager.h>
#include <flexy/net/hook.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test() {
    FLEXY_LOG_INFO(g_logger) << "test";
    sleep_f(10);
}

int main(int argc, char** argv) {
    flexy::IOManager iom(1);

    flexy::Signal::signal(SIGINT, [](int argc, char** argv){
        FLEXY_LOG_INFO(g_logger) << "SIGINT";
        for (int i = 1; i < argc; ++i) {
            FLEXY_LOG_DEBUG(g_logger) << argv[i];
        }
    }, argc, argv);
    iom.async(test);
}