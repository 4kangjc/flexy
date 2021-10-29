#pragma once

#include "flexy/thread/mutex.h"
#include "flexy/util/singleton.h"
#include <unordered_map>

namespace flexy {

class Env {
public:
    bool init(int argc, char** argv);
    void add(const std::string& key, const std::string& val);
    bool has(const std::string& key) const;
    void del(const std::string& key);
    std::string get(const std::string& key, const std::string& val = "") const;

    void addHelp(const std::string& key, const std::string& desc);
    void removeHelp(const std::string& key);
    void printHelp() const;

    auto& getExe() const { return exe_; }
    auto& getCwd() const { return cwd_; }

    bool setEnv(const std::string& key, const std::string& val);
    bool setEnv(const char* key, const char* val);
    std::string getEnv(const std::string& key, const std::string& default_value = "") const;
    std::string getEnv(const char* key, const std::string& default_value = "") const;

    std::string getAbsolutePath(std::string_view path) const;
private:
    mutable rw_mutex mutex_;
    std::unordered_map<std::string, std::string> args_;
    std::unordered_map<std::string, std::string> helps_;
    std::string program_;
    std::string exe_;
    std::string cwd_;
public:
    bool init_ = false;
};

using EnvMgr = Singleton<Env>;

} // namespace flexy