#include "flexy/schedule/worker.h"

static auto&& g_logger = FLEXY_LOG_ROOT();

class Work {
public:
    int print(int i) {
        FLEXY_LOG_INFO(g_logger) << "work! " << i;
        return i;
    }
    void operator()() { FLEXY_LOG_INFO(g_logger) << "functor!"; }
};

flexy::fiber::Semaphore fsem;

void test_sem1() {
    FLEXY_LOG_INFO(g_logger) << "test1 wait";
    fsem.wait();
    FLEXY_LOG_INFO(g_logger) << "test1 finish wait";
}

void test_sem2() {
    sleep(1);
    FLEXY_LOG_INFO(g_logger) << "test2 post";
    fsem.post();
    FLEXY_LOG_INFO(g_logger) << "test2 finish post";
}

void test_work_group() {
    auto wg = flexy::WorkerGroup::Create(2);
    Work w;
    wg->schedule([]() { FLEXY_LOG_INFO(g_logger) << "Hello worker!"; });
#if __cplusplus > 201703L
    // wg->schedule([] <class... Args> (fmt::format_string<Args...> fmt,
    // Args&&... args) { FLEXY_LOG_FMT_INFO(g_logger, fmt,
    // std::forward<Args>(args)...);
    // }, "{} {} {}", 1, 2, 3);
#else
    wg->schedule(
        [](const char* fmt, auto&&... args) {
            FLEXY_LOG_FMT_INFO(g_logger, fmt,
                               std::forward<decltype(args)>(args)...);
        },
        "{} {} {}", 1, 2, 3);
#endif

    wg->schedule([]() {

    });
    wg->schedule(&Work::print, &w, 1);
    wg->schedule(w);
    wg->schedule(test_sem1);
    wg->schedule(test_sem2);
}

int main() {
    flexy::IOManager iom(1);

    go test_work_group;
    // go test_sem1;
    // go test_sem2;
}