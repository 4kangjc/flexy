#pragma once
#include <string>
#include <vector>
#include <fstream>

namespace flexy {

// 文件系统 实现依靠C++17 filesystem
// v: 是否与与运行目录无关
struct FS {
    static std::string AbsolutePath(std::string_view filename, bool v = true);
    // 列出path路径下的所有以subfix结尾的文件
    static void ListAllFile(std::vector<std::string>& files, std::string_view path, 
                std::string_view subfix);
    static bool Mkdir(std::string_view dirname, bool v = false);
    static bool Rm(std::string_view path, bool v = false);
    static bool Mv(std::string_view from, std::string_view to, bool v = false); 
    static bool IsRunningPidfile(std::string_view pidfile);
    // open不支持std::string_view, 注意std::string_view 以 \\0 结尾
    static bool OpenForRead(std::ifstream& ifs, std::string_view  filename,
                std::ios_base::openmode mode);
    static bool OpenForWrite(std::ofstream& ofs, std::string_view filename,
                std::ios_base::openmode mode);
};


} // namespace flexy
