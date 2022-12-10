#include <gtest/gtest.h>
#include "flexy/util/log.h"
#include "flexy/util/task.h"

static auto&& g_logger = FLEXY_LOG_ROOT();

using namespace flexy::detail;

// using task = __task_virtual;
// using task = __task_function;
// using task = __task_template;

using task = __task_Function;

TEST(__Task, Lambda) {
    int ans = 0;
    task t1([](int a, int b, int& ans) { ans = a + b; }, 1, 2, std::ref(ans));
    t1();
    ASSERT_EQ(ans, 3);

    task t2([](const char* s) { ASSERT_EQ(s, "Hello World"); }, "Hello World");
    t2();
    t2();

    task t3([&ans](int a, int b) { ans = a * b; }, 4, 5);
    t3();
    ASSERT_EQ(ans, 20);

    task t5([ans]() mutable { ans /= 4; });
    ASSERT_EQ(ans, 20);

    task t6([](auto&& func) { func(); }, std::move(t2));

    task t7(
        [](auto&& first, auto&&... other) {
            auto res = (first + ... + other);
            ASSERT_EQ(res, 55);
        },
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    t7();
}

TEST(__Task, MemberMethod) {
    // noncopyable plus
    struct plus {
        int ans = 0;

        void operator()(int a, int b) { ans = a + b; }
        plus(const plus&) = delete;
        plus() = default;
        plus(plus&&) = default;
    };

    plus p;
    task t1(std::ref(p), 4, 5);
    t1();
    ASSERT_EQ(p.ans, 9);

    task t2(&plus::operator(), &p, 1, 2);
    t2();
    ASSERT_EQ(p.ans, 3);

    task t3(std::move(p), 0, 0);
    t3();

    struct Worker {
        void operator()() const {}

        void Work(std::string_view hello) { ASSERT_EQ(hello, "hello"); }
        Worker() = default;
        Worker(const Worker&) = default;
    };

    const Worker w;
    task t4(w);
    task t5(&Worker::operator(), &w);

    Worker ww;
    task t6(w);
    task t7(&Worker::Work, &ww, "hello");
}

void test_unique(std::unique_ptr<int>&& uptr) { ASSERT_EQ(*uptr, 0x99); }

void Invoke(task&& cb) { cb(); }

template <typename... Args>
void Forward(Args&&... args) {
    return Invoke(task(std::forward<Args>(args)...));
}

template <class T>
void max(T a, T b, T& ans) {
    ans = a < b ? a : b;
}

TEST(__Task, Function) {
    struct NonCopyable {
        void operator()() {}
        NonCopyable() = default;
        NonCopyable(NonCopyable&&) = default;
    };

    NonCopyable nc;
    task t1 = std::move(nc);
    t1();

    task t2(&test_unique, std::make_unique<int>(0x99));
    t2();

    Forward(test_unique, std::make_unique<int>(0x99));

    int ans = 0;
    task t3(&max<int>, 0x99, -2, std::ref(ans));
    t3();
    ASSERT_EQ(ans, -2);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}