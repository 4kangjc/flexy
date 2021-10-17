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
    struct EventContext {
        Scheduler* scheduler = nullptr;              // 事件执行的调度器
        Fiber::ptr fiber = nullptr;                 // 事件协程
        __task cb = nullptr;                        // 事件回调函数
    };
public:
    int handleEvents(Event events);
    void handleRead();
    void handleWrite();
    EventContext& getContext(Event event);
    auto& getReadContext() { return read_; }
    auto& getWriteContext() { return write_; }
    Channel(int epoll_fd, int fd, Event evens = Event::NONE);
    bool enableRead(bool enable = true);
    bool enableWrite(bool enable = true);
    bool enableEvents(Event events, bool enable = true);
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