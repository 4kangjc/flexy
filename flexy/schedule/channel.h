#pragma once
#include "flexy/util/noncopyable.h"
#include "flexy/thread/mutex.h"
#include "scheduler.h"

#include <iostream>

namespace flexy {

enum Event {
    NONE  = 0x0,
    READ  = 0x1,
    WRITE = 0x4
};

class Channel : noncopyable {
private:
    // 事件上下文
    struct EventContext {
        Scheduler* scheduler = nullptr;             // 事件执行的调度器
        Fiber::ptr fiber  = nullptr;                // 事件协程
        detail::__task cb = nullptr;                // 事件回调函数
    };
public:
    // 处理读写事件， 返回处理的事件数量
    int handleEvents(Event events);
    // 处理读事件
    void handleRead();
    // 处理写事件 
    void handleWrite();
    // 由event得到对应的事件上下文
    EventContext& getContext(Event event);
    // 得到读事件上下文
    auto& getReadContext() { return read_; }
    // 得到写事件上下文
    auto& getWriteContext() { return write_; }
    // 构造函数
    Channel(int epoll_fd, int fd, Event evens = Event::NONE);
    // 启用或取消读事件
    bool enableRead(bool enable = true);
    // 启用或取消写事件
    bool enableWrite(bool enable = true);
    // 启用或取消读写事件
    bool enableEvents(Event events, bool enable = true);
    // 重置事件上下文
    void resetContext(EventContext& ctx);
    int fd() const { return fd_; }
    Event getEvents() const { return events_; }
private:
    int epfd_;                                      // epoll 文件描述符
    int fd_;                                        // 事件关联的句柄
    Event events_ = Event::NONE;                    // 已经注册的事件
    EventContext read_;                             // 读事件
    EventContext write_;                            // 写事件 
public:
    mutable mutex mutex_;                           // 锁       // 由iomanager加锁
};

inline std::ostream& operator<<(std::ostream& os, Event events) {
    bool first = true;
#define XX(E) \
    if(events & E) { \
        if(!first) { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }
    XX(Event::NONE)
    XX(Event::READ) 
    XX(Event::WRITE)
#undef XX
    return os;
}

}