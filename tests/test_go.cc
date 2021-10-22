#include <flexy/schedule/iomanager.h>
#include <flexy/util/log.h>
#include <flexy/util/util.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void test(std::string_view s = "hello") {
    FLEXY_LOG_INFO(g_logger) << s;
}

void test1() {
    static int i = 5;
    FLEXY_LOG_INFO(g_logger) << "i = " << i;
    if (--i) {
        go test1;
    }
}

struct Add {
    void operator()(int a, int b) {
        FLEXY_LOG_INFO(g_logger) << "a + b = " << a + b;
    }
};

int main() {
    flexy::IOManager iom;
    go []() { 
        FLEXY_LOG_INFO(g_logger) << "Hello go"; 
    };
    go_args(test, "hello");
    Add add;
    go_args(&Add::operator(), &add, 1, 2);
    go_args(add, 1, 2);
    go test1;

}