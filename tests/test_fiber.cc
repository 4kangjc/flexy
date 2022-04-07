#include <flexy/schedule/fiber.h>
#include <flexy/util/log.h>
#include <flexy/util/memory.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

void run_in_fiber() {
    FLEXY_LOG_INFO(g_logger) << "run in fiber begin";
    flexy::Fiber::Yield();
    FLEXY_LOG_INFO(g_logger) << "run in fiber end";
}

int main() {
    FLEXY_LOG_INFO(g_logger) << "main begin -1";
    {
        flexy::Fiber::GetThis();
        FLEXY_LOG_INFO(g_logger) << "main begin";
        auto fiber = flexy::make_shared<flexy::Fiber>(run_in_fiber);
        fiber->resume();
        FLEXY_LOG_INFO(g_logger) << "main after call end";
        fiber->resume();
        FLEXY_LOG_INFO(g_logger) << "main end";
    }
    FLEXY_LOG_INFO(g_logger) << "main after end2";
}