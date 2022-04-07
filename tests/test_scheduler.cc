#include <flexy/schedule/scheduler.h>
#include <flexy/util/log.h>
#include <unistd.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void println(const char* s, int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        FLEXY_LOG_INFO(g_logger) << argv[i];
    }
    FLEXY_LOG_INFO(g_logger) << s;
}

void test() {
    static int s_count = 5;
    FLEXY_LOG_FMT_INFO(g_logger, "test s_count = {}", s_count); 
    if (--s_count >= 0) {
        flexy::Scheduler::GetThis()->async(test);
    }
}

int main(int argc, char** argv) {
    flexy::Scheduler sc(1, true, "TEST");
    sc.async(println, "hello scheduler", argc, argv); 
    sc.start();
    // sleep(1);
    sc.async_first(nullptr);
    sc.async_first(test);
    
    sc.async([](auto&& first, auto&&... other){
        FLEXY_LOG_INFO(g_logger) << "sum = " << (first + ... + other);
    }, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    sc.stop();
}