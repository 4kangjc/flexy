#include <gtest/gtest.h>
#include <string>
#include "flexy/util/file.h"
#include "flexy/util/log.h"

static auto&& g_logger = FLEXY_LOG_ROOT();

// TEST(File, AbsolutePath) {
//     ASSERT_EQ(flexy::filesystem::AbsolutePath("~"), getenv("HOME"));
//     FLEXY_LOG_INFO(g_logger) << flexy::FS::AbsolutePath(".");
//     ASSERT_EQ(flexy::filesystem::AbsolutePath("/bin"), "/bin");
// }

TEST(File, Mkdir) {
    ASSERT_TRUE(flexy::filesystem::Mkdir("temp"));
    ASSERT_TRUE(flexy::filesystem::Mkdir("temp/kkk/mm"));
}

TEST(File, Mv) {
    ASSERT_TRUE(flexy::filesystem::Mv("temp/kkk/mm", "temp/kkk/fff"));
}

TEST(File, Rm) {
    ASSERT_TRUE(flexy::filesystem::Rm("temp"));
    ASSERT_TRUE(flexy::filesystem::Rm("temp/kkk/fff"));
}

TEST(File, ListAllFile) {
    std::vector<std::string> vec;
    flexy::FS::ListAllFile(vec, "../flexy", ".cpp");
    for (auto& file : vec) {
        FLEXY_LOG_INFO(g_logger) << file;
    }
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}