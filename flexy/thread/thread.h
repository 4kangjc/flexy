#pragma once

#include "semaphore.h"
#include "flexy/util/task.h"

namespace flexy {

class Thread : noncopyable {
public:
    using ptr = std::shared_ptr<Thread>;
    // 线程构造函数
    template <typename... Args>
    Thread(std::string_view name, Args&&... args) : name_(name), 
            cb_(std::forward<Args>(args)...) {
        if (name.empty()) {
            name_ = "UNKOWN";
        }
        pthread_create(&thread_, nullptr, &Thread::run, this);   
        semapthore_.wait();
    }
    ~Thread();
    // 返回线程真实id
    pid_t getId() const { return tid_; }
    // 返回线程名称
    const std::string& getName() const { return name_; }
    // 等待线程结束
    void join();
    // 返回当期线程
    static Thread* GetThis();
    // 返回当前线程的名称
    static const std::string& GetName();
    // 设置当前线程的名称
    static void SetName(std::string_view name);  
    // 返回当前线程开始的毫秒数 不包括主线程
    static uint64_t GetStartTime();
    // 返回当前线程真实id
    static pid_t GetThreadId();
private:
    static void* run(void* arg);                 // 线程真正执行函数
private:
    pid_t tid_ = -1;                            // 线程真实id
    pthread_t thread_;                          // 线程结构
    std::string name_;                          // 线程名称
    __task cb_;                                 // 线程执行函数
    Semaphore semapthore_;                      // 信号量
};

} // namespace flexy