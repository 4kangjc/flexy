#pragma once
#include <string>
#include <vector>
#include <fstream>

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

template <bool v = true>
std::string AbsolutePath(std::string_view filename);

// 列出path路径下的所有以subfix结尾的文件
void ListAllFile(std::vector<std::string>& files, std::string_view path, 
                 std::string_view subfix);

// 建立文件夹
template <bool v = false>
bool Mkdir(std::string_view dirname);

// 删除文件或文件夹
template <bool v = false>
bool Rm(std::string_view path);

// 移动或重命名文件或文件夹
template <bool v = false>
bool Mv(std::string_view from, std::string_view to);

bool IsRunningPidfile(std::string_view pidfile);

// open不支持std::string_view, 注意std::string_view 以 \\0 结尾
template <bool v = false>
bool OpenForRead(std::ifstream& ifs, std::string_view  filename,
                 std::ios_base::openmode mode);

// open不支持std::string_view, 注意std::string_view 以 \\0 结尾
template <bool v = false>
bool OpenForWrite(std::ofstream& ofs, std::string_view filename,
                  std::ios_base::openmode mode);

// 上次文件修改时间
template <bool v = false>
uint64_t LastWriteTime(std::string_view filename);

} // namespace flexy file system
