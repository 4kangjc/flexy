#include <gtest/gtest.h>
#include <vector>
#include "flexy/fiber/fiber.h"
#include "flexy/util/log.h"
#include "flexy/util/memory.h"

static auto&& g_logger = FLEXY_LOG_ROOT();

std::vector<int> indexs;

void run_in_fiber1() {
    indexs.push_back(2);

    flexy::Fiber::Yield();

    indexs.push_back(11);
}

void run_in_fiber2() {
    indexs.push_back(4);
    // flexy::Fiber::Yield();
    flexy::Fiber::GetThis()->yield_callback([]() { indexs.push_back(5); });

    indexs.push_back(7);
}

void run_in_fiber3() { indexs.push_back(9); }

TEST(Fiber, Main) {
    indexs.push_back(0);
    {
        flexy::Fiber::GetThis();

        indexs.push_back(1);

        auto fiber1 = flexy::fiber_make_shared(run_in_fiber1);
        auto fiber2 = flexy::fiber_make_shared(run_in_fiber2);
        ASSERT_EQ(fiber1->getState(), flexy::Fiber::READY);
        ASSERT_EQ(fiber2->getState(), flexy::Fiber::READY);

        fiber1->resume();

        indexs.push_back(3);
        fiber2->resume();

        indexs.push_back(6);

        fiber2->resume();

        indexs.push_back(8);

        ASSERT_EQ(fiber2->getState(), flexy::Fiber::TERM);
        fiber2->reset(run_in_fiber3);

        fiber2->resume();
        ASSERT_EQ(fiber2->getState(), flexy::Fiber::TERM);

        indexs.push_back(10);

        fiber1->resume();
        ASSERT_EQ(fiber1->getState(), flexy::Fiber::TERM);
    }
    indexs.push_back(12);

    for (int i = 0; i < (int)indexs.size(); ++i) {
        ASSERT_EQ(indexs[i], i);
    }
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}