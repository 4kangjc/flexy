#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace flexy {

// 获得当前线程真实id
int GetThreadId();
// 获取线程启动到现在的毫秒
uint32_t GetThreadElapse();
// 获得线程名称
const std::string& GetThreadName();
// 获得协程id
uint32_t GetFiberId();
// 获得当前系统时间微秒数
uint64_t GetTimeUs();
// 获得当前系统时间毫秒数
uint64_t GetTimeMs();
// 获得当前Steady时钟微秒数
uint64_t GetSteadyUs();
// 获得当前Steady时钟毫秒数
uint64_t GetSteadyMs();
// 时间秒数 装换为 时间
std::string TimeToStr(time_t ts = time(0), const std::string& fmt = "%Y-%m-%d %H:%M:%S");
// c语言格式化字符串 -> std::string
std::string format(const char* fmt, ...);
// 查找元素 [begin, end) 找不到返回end, find_first -> 是否从前往后找
template <typename Iter, typename T>
Iter find(Iter&& begin, Iter&& end, T&& val, bool find_first = true) {
    if (find_first) {
        for (auto it = begin; it != end; ++it) {
            if (*it == val) {
                return it;
            }
        }
    } else {
        for (auto it = end - 1; it != begin; --it) {
            if (*it == val) {
                return it;
            }
        }
    }
    return end;
}
// 将[begin, end)范围中的字符转换为整数
int64_t atoi(const char* begin, const char* end);
//获得函数调用堆栈信息
void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
//获得函数调用堆栈信息字符串
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

}