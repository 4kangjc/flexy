#include "env.h"
#include "flexy/util/log.h"
#include "flexy/util/file.h"
// #include <filesystem>
#include <iostream>
#include <unistd.h>
#include <iomanip>

// namespace fs = std::filesystem;

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

bool Env::init(int argc, char** argv) {
    init_ = true;
    program_ = argv[0];
    // exe_ = fs::current_path()/program_;      // 环境变量时不对

    char path[1024] = {0};
    [[maybe_unused]] auto res = readlink("/proc/self/exe", path, sizeof(path));
    exe_ = path;

    auto pos = exe_.find_last_of("/") + 1;
    cwd_ = exe_.substr(0, pos);
    
    const char* now_key = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (strlen(argv[i]) > 1) {
                if (now_key) {
                    add(now_key, "");
                }
                now_key = argv[i] + 1;
            } else {
                FLEXY_LOG_ERROR(g_logger) << "invalid arg idx = " << i
                << " val = " << argv[i];
                return false;
            }
        } else {
            if (now_key) {
                add(now_key, argv[i]);
                now_key = nullptr;
            } else {
                FLEXY_LOG_ERROR(g_logger) << "invalid arg idx = " << i
                << " val = " << argv[i];
                return false;
            }
        }
    }

    if (now_key) {
        add(now_key, "");
    }

    return true;
}

void Env::add(const std::string& key, const std::string& val) {
    WRITELOCK(mutex_);
    args_.emplace(key, val);
}

bool Env::has(const std::string& key) const {
    READLOCK(mutex_);
    return args_.find(key) != args_.end();
}

void Env::del(const std::string& key) {
    WRITELOCK(mutex_);
    args_.erase(key);
}

std::string Env::get(const std::string& key, const std::string& default_val) const {
    READLOCK(mutex_);
    auto it = args_.find(key);
    return it != args_.end() ? it->second : default_val;
}

void Env::addHelp(const std::string& key, const std::string& desc) {
    WRITELOCK(mutex_);
    helps_.emplace(key, desc);
}

void Env::removeHelp(const std::string& key) {
    WRITELOCK(mutex_);
    helps_.erase(key);
}

void Env::printHelp() const {
    READLOCK(mutex_);
    std::cout << "Usage: " << program_ << " [options]" << std::endl;
    for (const auto& [x, y] : helps_) {
        std::cout << std::setw(5) << "-" << x << " : " << y << std::endl;
    }
}

bool Env::setEnv(const std::string& key, const std::string& val) {
    return !setenv(key.c_str(), val.c_str(), 1);
}

bool Env::setEnv(const char* key, const char* val) {
    return !setenv(key, val, 1);
}

std::string Env::getEnv(const std::string& key, const std::string& default_value) const {
    const char* v = getenv(key.c_str());
    if (v == nullptr) {
        return default_value;
    }
    return v;
}

std::string Env::getEnv(const char* key, const std::string& default_value) const {
    const char* v = getenv(key);
    if (v == nullptr) {
        return default_value;
    }
    return v;
}

std::string Env::getAbsolutePath(std::string_view path) const {
    if(path.empty()) {
        return "/";
    }
    if(path[0] == '/') {
        return std::string(path);
    }
    return cwd_ + std::string(path);
}

}