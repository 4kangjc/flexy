#include <flexy/schedule/iomanager.h>
#include <flexy/util/log.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test_timer() {
    flexy::IOManager iom(2);
    flexy::Timer::ptr timer = iom.addRecTimer(1000, [&timer]() {
        static int i = 0;
        FLEXY_LOG_INFO(g_logger) << "hello timer";
        if (++i == 5) {
            timer->cacel();
        }
    });
}

int main() {
    test_timer();
}