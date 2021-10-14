#include <flexy/util/file.h>
#include <flexy/util/log.h>

static auto g_logger = FLEXY_LOG_ROOT();

int main(int argc, char** argv) {
    FLEXY_LOG_INFO(g_logger) << flexy::FS::AbsolutePath("../bin/log.txt");
    FLEXY_LOG_FMT_INFO(g_logger, "{}", argv[0]);
    FLEXY_LOG_INFO(g_logger) << flexy::FS::AbsolutePath(argv[0], false);
    FLEXY_LOG_INFO(g_logger) << flexy::FS::AbsolutePath(".");
    FLEXY_LOG_INFO(g_logger) << flexy::FS::AbsolutePath("/bin");
    flexy::FS::Mv("../bin/conf/log.txt", "../bin/log.txt", true);
    std::vector<std::string> vec;
    flexy::FS::ListAllFile(vec, "../flexy", ".cpp");
    for (auto& file : vec) {
        FLEXY_LOG_INFO(g_logger) << file;
    } 
    bool v = flexy::FS::Mkdir("../bin/temp", true);
    if (!v) {
        FLEXY_LOG_INFO(g_logger) << "mkdir failed";
    }
    v = flexy::FS::Rm("../bin/temp/", true);
    if (!v) {
        FLEXY_LOG_INFO(g_logger) << "rm -rf failed";
    }
}