#include <flexy/net/hook.h>
#include <flexy/util/log.h>
#include <flexy/schedule/iomanager.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test_sleep() {
    flexy::IOManager iom(1);
    iom.async([](){
        sleep(2);
        FLEXY_LOG_INFO(g_logger) << "sleep 2";
    });
    iom.async([](){
        sleep(3);
        FLEXY_LOG_INFO(g_logger) << "sleep 3";
    });
    FLEXY_LOG_INFO(g_logger) << "test_sleep";
}

int main() {
    test_sleep();
}