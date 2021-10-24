#include <flexy/schedule/iomanager.h>
#include <flexy/util/log.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test_timer() {
    flexy::Timer::ptr timer;
    flexy::IOManager iom(2);
    timer = iom.addRecTimer(1000, [](flexy::Timer::ptr& t) {
        static int i = 0;
        FLEXY_LOG_INFO(g_logger) << "hello timer";
        if (++i == 5) {
            t->cancel();
        }
    }, std::ref(timer));
}

int main() {
    test_timer();

    flexy::IOManager iom(1);
    iom.addTimer(1000, [](auto&&... args) {
        (std::cout << ... << args) << std::endl;
    }, "Hello Timer", " World Hello", 1, 31.0f, 4520ull);
}