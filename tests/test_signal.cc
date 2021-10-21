#include <flexy/env/signal.h>
#include <flexy/util/log.h>
#include <flexy/schedule/iomanager.h>
#include <signal.h>
#include <unistd.h>
#include <flexy/net/hook.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test() {
    FLEXY_LOG_INFO(g_logger) << "test";
    sleep_f(10);
}

int main() {
    flexy::IOManager iom(1);

    flexy::Signal::signal(SIGINT, [](){
        FLEXY_LOG_INFO(g_logger) << "SIGINT";
    });
    iom.async(test);
}