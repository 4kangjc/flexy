#pragma once
#include <bits/types/FILE.h>
#include <unistd.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include "flexy/util/log.h"

namespace flexy {

// 文件系统 实现依靠C++17 filesystem
// v: 是否与与运行目录无关
struct FS {
    // 得到绝对路径
    static std::string AbsolutePath(std::string_view filename, bool v = true);
    // 列出path路径下的所有以subfix结尾的文件
    static void ListAllFile(std::vector<std::string>& files, std::string_view path, 
                std::string_view subfix);
    // 建立文件夹
    static bool Mkdir(std::string_view dirname, bool v = false);
    // 删除文件或文件夹
    static bool Rm(std::string_view path, bool v = false);
    // 移动或重命名文件或文件夹
    static bool Mv(std::string_view from, std::string_view to, bool v = false); 
    static bool IsRunningPidfile(std::string_view pidfile);
    // open不支持std::string_view, 注意std::string_view 以 \\0 结尾
    static bool OpenForRead(std::ifstream& ifs, std::string_view  filename,
                std::ios_base::openmode mode);
    // open不支持std::string_view, 注意std::string_view 以 \\0 结尾
    static bool OpenForWrite(std::ofstream& ofs, std::string_view filename,
                std::ios_base::openmode mode);
    // 上次文件修改时间
    static uint64_t LastWriteTime(std::string_view filename); 
};


} // namespace flexy

// 与 std::filesystem的不同地方在于
// 若`v`为`true`那么 path 会转换为 绝对路径
// 且 绝对路径不会随着工作目录的变更而变更 始终是相对于`bin`目录下
namespace flexy::filesystem {

using namespace std::filesystem;

inline auto g_logger = FLEXY_LOG_NAME("system");

template <bool v = true>
std::string AbsolutePath(std::string_view filename) {
    if (filename[0] == '/') {
        return std::string(filename);
    } else if (filename[0] == '~') {
        const char* home = getenv("HOME");
        std::string file = home ? home : "";
        file += filename.substr(1);
        return file;
    }
    if constexpr (v) {
        char path[512] = {0};
        ssize_t count = readlink("/proc/self/exe", path, 512);
        if (count <= 0) {
            return "";
        }
        auto p = filesystem::path(path).parent_path() / filename;
        return p;
    }
    return absolute(filename);
}

// 建立文件夹
template <bool v = false>
bool Mkdir(std::string_view dirname) {
    if (v) {
        auto s = AbsolutePath(dirname);
        return Mkdir(s);
    }
    filesystem::path dir(dirname);
    if (exists(dir)) {
        return true;
    }
    return create_directories(dirname);
}

// 删除文件或文件夹
template <bool v = false>
bool Rm(std::string_view path) {
    if (v) {
        auto s = AbsolutePath(path);
        return Rm(s);
    }
    try {
        remove_all(path);
    } catch (std::exception& ex) {
        FLEXY_LOG_FMT_ERROR(g_logger, "filesystem Rm Except: {}", ex.what());
        return false;
    } catch (...) {
        FLEXY_LOG_ERROR(g_logger) << "filesystem Rm Except";
        return false;
    }
    return true;
}

// 移动或重命名文件或文件夹
template <bool v = false>
bool Mv(std::string_view from, std::string_view to) {
    try {
        if constexpr (v) {
            auto __from = AbsolutePath(from);
            auto __to = AbsolutePath(to);
            rename(__from, __to);
        } else {
            rename(from, to);
        }
    } catch (std::exception& ex) {
        FLEXY_LOG_FMT_ERROR(g_logger, "FS Mv Except: {}", ex.what());
        return false;
    } catch (...) {
        FLEXY_LOG_ERROR(g_logger) << "FS Mv Except";
        return false;
    }
    return true;
}

bool IsRunningPidfile(std::string_view pidfile);

// open不支持std::string_view, 注意std::string_view 以 \\0 结尾
template <bool v = false>
bool OpenForRead(std::ifstream& ifs, std::string_view filename,
                 std::ios_base::openmode mode) {
    if constexpr (v) {
        return FS::OpenForRead(ifs, AbsolutePath(filename), mode);
    }
    ifs.open(filename.data(), mode);
    return ifs.is_open();
}

// open不支持std::string_view, 注意std::string_view 以 \\0 结尾
template <bool v = false>
bool OpenForWrite(std::ofstream& ofs, std::string_view filename,
                  std::ios_base::openmode mode) {
    if constexpr (v) {
        return FS::OpenForWrite(ofs, AbsolutePath(filename), mode);
    }
    ofs.open(filename.data(), mode);
    if (!ofs.is_open()) {
        path file(filename);
        create_directories(file.parent_path());
        ofs.open(filename.data(), mode);
    }
    return ofs.is_open();
}

// 上次文件修改时间
template <bool v = false>
uint64_t LastWriteTime(std::string_view filename) {
    if constexpr (v) {
        return FS::LastWriteTime(AbsolutePath(filename));
    }
    auto ftime = last_write_time(filename);
    return std::chrono::duration_cast<std::chrono::microseconds>(
               ftime.time_since_epoch())
        .count();
}

}  // namespace flexy::filesystem
