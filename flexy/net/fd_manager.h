#pragma once

#include <vector>
#include "flexy/thread/mutex.h"
#include "flexy/util/singleton.h"

namespace flexy {

// 文件句柄上下文
class FdCtx {
public:
    FdCtx(int fd);
    ~FdCtx();
    // 
    bool init();
    // 是否初始化完成
    bool isInit() const { return isInit_; }
    // 是否是socket
    bool isSocket() const { return isSocket_; }
    // 是否关闭
    bool isClose() const { return isClosed_; }
    // 设置用户态是否为阻塞
    void setUserNonblock(bool v) { userNonblock_ = v; }
    // 用户是否设置为非阻塞
    bool getUserNonblock() const { return userNonblock_; }
    // 设置系统(hook)是否为阻塞
    void setSysNonblock(bool v) { sysNonblock_ = v; }
    // 系统(hook)是否为阻塞
    bool getSysNonblock() const { return sysNonblock_; }
    // 设置type超时时间
    void setTimeout(int type, uint64_t v);
    // 获取type超时时间
    uint64_t getTimeout(int type);
private:
    bool isInit_ : 1;                       // 是否初始化
    bool isSocket_ : 1;                     // 是否socket
    bool sysNonblock_ : 1;                  // 是否hook非阻塞
    bool userNonblock_ : 1;                 // 是否用户主动设置非阻塞
    bool isClosed_ : 1;                     // 是否关闭
    int fd_;                                // 文件句柄
    uint64_t recvTimeout_;                  // 读超时时间 (ms)
    uint64_t sendTimeout_;                  // 写超时时间
};


class FdManager {
public:
    FdManager();
    ~FdManager();
    FdCtx* get(int fd, bool auto_create = false);      // 获取/创建 文件句柄
    void del(int fd);                                  // 删除文件句柄
private:
    mutable mutex mutex_;                   // 锁
    std::vector<FdCtx*> datas_;             // 文件句柄集合
};

using FdMsg = Singleton<FdManager>;

} // namespace flexy