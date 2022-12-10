#include <flexy/fiber/this_fiber.h>

using namespace flexy;
using namespace std::chrono_literals;

static auto& g_logger = FLEXY_LOG_ROOT();

void test_this_fiber() {
    FLEXY_LOG_INFO(g_logger)
        << "hello this fiber, id = " << this_fiber::get_id();
    // IOManager::GetThis()->async(Fiber::GetThis());
    // this_fiber::yield();
    FLEXY_LOG_DEBUG(g_logger) << "begin sleep";

    this_fiber::sleep_for(2000ms);
    FLEXY_LOG_DEBUG(g_logger) << "end sleep";
}

void work() {
    FLEXY_LOG_DEBUG(g_logger) << "work...";
    this_fiber::sleep_for(3s);
    FLEXY_LOG_DEBUG(g_logger) << "work finish!";
}

void test_sleep_util() {
    FLEXY_LOG_FMT_DEBUG(g_logger, "welcome to sleep util");
    const auto start(std::chrono::steady_clock::now());
    this_fiber::sleep_until(start + 2000ms);
}

int main() {
    IOManager iom;
    go test_this_fiber;
    go work;
    go test_sleep_util;
    FLEXY_LOG_INFO(g_logger) << "main fiber id = " << this_fiber::get_id();
}