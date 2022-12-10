#include <gtest/gtest.h>
#include "flexy/util/function.h"
#include "flexy/util/log.h"

static auto g_logger = FLEXY_LOG_ROOT();

int minus(int a, int b) { return a - b; }

int product(int a, int b) { return a * b; }

struct divides {
    int operator()(int a, int b) const { return a / b; }
};

// noncopyable plus
struct plus {
    int operator()(int a, int b) const { return a + b; }
    plus(const plus&) = delete;
    plus() = default;
    plus(plus&&) = default;
};

TEST(Function, Empty) { flexy::Function<void()> f; }

TEST(Function, Lambda) {
    flexy::Function f1 = [](int a, int b) { return a + b; };
    ASSERT_EQ(3, f1(1, 2));

    int a = 0;
    flexy::Function f2 = [&a]() { ++a; };
    f2();
    ASSERT_EQ(a, 1);

    plus p;
    flexy::Function f3([](plus&& p) { return p(2, 5); });
    ASSERT_EQ(f3(std::move(p)), 7);

    divides d;
    flexy::Function f4([d](int a, int b) { return d(a, b); });
    ASSERT_EQ(f4(20, 5), 4);

    std::unique_ptr uq = std::make_unique<int>(3);
    flexy::Function f5 = [uni_ptr = std::move(uq)]() { return *uni_ptr; };
    ASSERT_EQ(f5(), 3);

    std::array<char, 1000000> payload;
    payload.back() = 5;
    flexy::Function f6 = [payload] { return payload.back(); };
    ASSERT_EQ(f6(), 5);

    // auto lamda = []() {

    // };
    // flexy::Function<decltype(lamda)> f7;
}

struct Pii {
    int a, b;
    int hash_func() { return std::hash<int>()(a) ^ std::hash<int>()(b); }
    int sum(int c = 0) const { return a + b + c; }
};

TEST(Function, MemberMethod) {
    plus p;
    flexy::Function f1(std::move(p));
    ASSERT_EQ(f1(2, 5), 7);

    divides div;
    flexy::Function f2(div);
    ASSERT_EQ(f2(30, 5), 6);

    Pii pii{4, 5};
    flexy::Function<int(Pii*)> f3(&Pii::hash_func);
    ASSERT_EQ(f3(&pii), pii.hash_func());

    flexy::Function<int(Pii*, int)> f4(&Pii::sum);
    ASSERT_EQ(f4(&pii, 1), 10);
}

template <class T>
void multiplication(T& a, const T& b) {
    a *= b;
}

TEST(Function, Function) {
    flexy::Function f1(minus);
    ASSERT_EQ(f1(10, 5), 5);

    flexy::Function f2 = &product;
    ASSERT_EQ(f2(2, 3), 6);

    flexy::Function f3(&multiplication<double>);
    double a = 4.0f;
    f3(a, 5);
    ASSERT_EQ(a, 20);
}

TEST(Function, Bind) {
    flexy::Function<int(int)> f1 = std::bind(minus, 10, std::placeholders::_1);
    ASSERT_EQ(f1(5), 5);

    Pii pii{2, 3};
    // flexy::Function<int(int)> f2(std::bind(&Pii::sum, &pii,
    // std::placeholders::_1));

    std::function<int(int)> f2(
        std::bind(&Pii::sum, &pii, std::placeholders::_1));
    ASSERT_EQ(f2(0), 5);
}

TEST(Function, Swap) {
    flexy::Function f1(product);
    ASSERT_EQ(f1(2, 5), 10);
    flexy::Function([](int a, int b) { return a + b; }).swap(f1);
    ASSERT_EQ(f1(2, 5), 7);

    flexy::Function f2(&minus);
    ASSERT_EQ(f2(3, 2), 1);
    Pii pii{4, 5};
    flexy::Function f3([pii](int a, int b) { return pii.sum(a) + b; });
    ASSERT_EQ(f3(1, 0), 10);
    f3.swap(f2);
    ASSERT_EQ(f3(3, 2), 1);
    ASSERT_EQ(f2(1, 0), 10);
}

// flare test
TEST(Function, FunctorMoveTest) {
    struct OnlyCopyable {
        OnlyCopyable() : v(new std::vector<int>()) {}
        OnlyCopyable(const OnlyCopyable& oc) : v(new std::vector<int>(*oc.v)) {}
        ~OnlyCopyable() { delete v; }
        std::vector<int>* v;
    };
    flexy::Function<int()> f, f2;
    OnlyCopyable payload;

    payload.v->resize(100, 12);

    // BE SURE THAT THE LAMBDA IS NOT LARGER THAN kMaximumOptimizableSize.
    f = [payload] { return payload.v->back(); };
    f2 = std::move(f);
    ASSERT_EQ(12, f2());
}

TEST(Function, LargeFunctorMoveTest) {
    flexy::Function<int()> f, f2;
    std::array<std::vector<int>, 100> payload;

    payload.back().resize(10, 12);
    f = [payload] { return payload.back().back(); };
    f2 = std::move(f);
    ASSERT_EQ(12, f2());
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}