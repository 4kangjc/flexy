#include <flexy/env/env.h>
#include <flexy/util/file.h>
#include <flexy/util/log.h>
#include <iostream>

static auto&& g_logger = FLEXY_LOG_ROOT();

int main(int argc, char** argv) {
    flexy::EnvMgr::GetInstance().addHelp("s", "start with the terminal");
    flexy::EnvMgr::GetInstance().addHelp("d", "run as daemon");
    flexy::EnvMgr::GetInstance().addHelp("p", "print help");
    if (!flexy::EnvMgr::GetInstance().init(argc, argv)) {
        flexy::EnvMgr::GetInstance().printHelp();
        return 0;
    }
    std::cout << "exe = " << flexy::EnvMgr::GetInstance().getExe() << std::endl;
    std::cout << "cwd = " << flexy::EnvMgr::GetInstance().getCwd() << std::endl;
    std::cout << "path = " << flexy::EnvMgr::GetInstance().getEnv("PATH", "xxx")
              << std::endl;
    std::cout << "test = " << flexy::EnvMgr::GetInstance().getEnv("test")
              << std::endl;
    std::cout << "set env "
              << flexy::EnvMgr::GetInstance().setEnv("test", "yyy")
              << std::endl;
    std::cout << "test = " << flexy::EnvMgr::GetInstance().getEnv("test")
              << std::endl;

    if (flexy::EnvMgr::GetInstance().has("p")) {
        flexy::EnvMgr::GetInstance().printHelp();
    }

    std::cout << "------------------------------------------------\n";

    auto ss = flexy::EnvMgr::GetInstance().getAbsolutePath("conf");
    std::cout << ss << std::endl;

    std::vector<std::string> files;
    flexy::FS::ListAllFile(files, ss, ".yml");

    std::cout << "total has " << files.size() << " files\n";

    for (auto& str : files) {
        std::cout << str << std::endl;
    }

    return 0;
}