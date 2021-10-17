#include "flexy/util/task.h"
#include "flexy/util/log.h"

static auto&& g_logger = FLEXY_LOG_ROOT();


void print(const char* s) {
    FLEXY_LOG_FMT_INFO(g_logger, "{}", s);
}

struct Add {
    void operator() (int a, int b) {
        FLEXY_LOG_FMT_INFO(g_logger, "{} + {} = {}", a, b, a + b);
    }
};

void test2(__task&& cb) {
    cb();
}

template <typename... Args>
void test1(Args&&... args) {
    // __task cb(std::forward<Args>(args)...);
    return test2(__task(std::forward<Args>(args)...));
    // return test2(std::move(cb));
}


int main() {
    __task t1(print, "hello __task");
    __task t2(print, "hello __task again");
    t1();
    t2();
    __task t3([]() { FLEXY_LOG_FMT_INFO(g_logger, "test lamada"); });
    t3();
    int i = 2;
    __task t4([&i]() { ++i; });
    t4();
    FLEXY_LOG_FMT_INFO(g_logger, "i = {}", i);
    Add a;
    __task t5(&Add::operator(), a, 1, 2);
    __task t6(a, 2, 3);
    t5(); t6();

    test1(print, "Hello std::forward");
}