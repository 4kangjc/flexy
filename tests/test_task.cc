#include <flexy/util/task.h>
#include <flexy/util/log.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

using namespace flexy::detail;

// using task = __task_virtual;
// using task = __task_function;
using task = __task_template;

void print(const char* s) {
    FLEXY_LOG_FMT_INFO(g_logger, "{}", s);
}

struct Add {
    void operator() (int a, int b) {
        FLEXY_LOG_FMT_INFO(g_logger, "{} + {} = {}", a, b, a + b);
    }
};

void test2(task&& cb) {
    cb();
}

template <typename... Args>
void test1(Args&&... args) {
    // task cb(std::forward<Args>(args)...);
    return test2(task(std::forward<Args>(args)...));
    // return test2(std::move(cb));
}

void test_unique(std::unique_ptr<int>&& uptr) {
    FLEXY_LOG_DEBUG(g_logger) << "unique_ptr value = " << *uptr;
}

int main() {
    task t1(print, "hello task");
    task t2(print, "hello task again");
    t1();
    t2();
    task t3([]() { FLEXY_LOG_FMT_INFO(g_logger, "test lamada"); });
    t3();
    int i = 2;
    task t4([&i]() { ++i; });
    t4();
    FLEXY_LOG_FMT_INFO(g_logger, "i = {}", i);
    Add a;
    task t5(&Add::operator(), a, 1, 2);
    task t6(a, 2, 3);
    t5(); t6();

    test1(print, "Hello std::forward");

    std::unique_ptr<int> uptr(new int(2));
    task t7(test_unique, std::move(uptr));
    t7();
}