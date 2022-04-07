#pragma once

#include "channel.h"
#include "timer.h"

namespace flexy {

class IOManager : public Scheduler, public TimerManager {
public:
    IOManager(size_t thread = 1, bool use_caller = true, std::string_view name = "");
    ~IOManager();
    // 添加事件及其回调函数
    template <typename... Args>
    bool addEvent(int fd, Event events, Args&&... args) {
        return onEvent(fd, events, detail::__task(std::forward<Args>(args)...));
    }
    // 删除事件及其回调函数
    bool delEvent(int fd, Event event);
    // 删除读事件及其回调函数
    bool delRead(int fd) { return delEvent(fd, Event::READ); }
    // 删除写事件及其回调函数
    bool delWrite(int fd) { return delEvent(fd, Event::WRITE); }
    // 立即执行事件回调函数，取消事件
    bool cancelEvent(int fd, Event event);
    // 若有读事件取消读事件，若有写事件取消写事件
    bool cancelAll(int fd);
    // 取消读事件
    bool cancelRead(int fd) { return cancelEvent(fd, Event::READ); }
    // 取消写事件
    bool cancelWrite(int fd) { return cancelEvent(fd, Event::WRITE); }
    // 添加事件及其回调函数
    bool onEvent(int fd, Event event, detail::__task&& cb = nullptr);

    // 注册读事件
    template <typename... Args>
    bool onRead(int fd, Args&&... args) {
        return onEvent(fd, Event::READ, detail::__task(std::forward<Args>(args)...));
    }
    // 注册写事件
    template <typename... Args>
    bool onWrite(int fd, Args&&... args) {
        return onEvent(fd, Event::WRITE, detail::__task(std::forward<Args>(args)...)); 
    }
    // 返回当前的IOManager
    static IOManager* GetThis();
protected:
    [[deprecated]] void tickle() override;
    bool stopping() override;
    void idleFiber();
    [[deprecated]] void idle() override;
    [[deprecated]] void onTimerInsertedAtFront() override;
    // 判断是否可以停止
    bool stopping(uint64_t& timeout);
    // 对 Channel 集合容器扩容
    void channelResize(size_t size);
private:
    int epfd_;                                                  // epoll文件描述符
    int tickleFds_[2];                                          // 管道 用作tickle
    std::atomic<size_t> pendingEventCount_ = {0};               // 当前等待执行的事件数量
    mutable mutex mutex_;                                       //  锁 
    std::vector<Channel*> channels_;                            // Channel集合
};

} // namespace flexy