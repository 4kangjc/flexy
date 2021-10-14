#include <flexy/thread/thread.h>
#include <flexy/util/log.h>

static auto g_logger = FLEXY_LOG_ROOT();

void test_thread(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        // printf("%s\n", argv[i]);
        FLEXY_LOG_FMT_INFO(g_logger, "{}", argv[i]);
    }
}

struct Add {
    void operator()(int a, int b) {
        FLEXY_LOG_FMT_INFO(g_logger, "{}", a + b);
    }
};

void print() {
    //printf("Hello World!");
    FLEXY_LOG_INFO(g_logger) << "Hello World!";
}

int main(int argc, char** argv) {
    flexy::Thread t("hello", test_thread, argc, argv);
    flexy::Thread t2("print", print);
    Add a;
    flexy::Thread t3("add", &Add::operator(), a, 1, 2);
    t.join();
    t2.join();
    t3.join();
}