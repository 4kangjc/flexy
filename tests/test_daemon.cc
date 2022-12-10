#include <flexy/env/daemon.h>
#include <flexy/schedule/iomanager.h>
#include <flexy/util/log.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

int server_main(int argc, char** argv) {
    FLEXY_LOG_INFO(g_logger) << flexy::ProcessInfoMrg::GetInstance().toString();
    flexy::Timer::ptr timer;
    flexy::IOManager iom;
    timer = iom.addRecTimer(1000, [&timer]() {
        FLEXY_LOG_INFO(g_logger) << "onTimer";
        static int count = 0;
        if (++count > 10) {
            timer->cancel();
        }
    });
    return 0;
};

int main(int argc, char** argv) {
    return flexy::start_daemon(argc, argv, server_main, argc != 1);
}