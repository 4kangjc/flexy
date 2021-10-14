#pragma once

#include "semaphore.h"
#include "flexy/util/task.h"

namespace flexy {

class Thread : noncopyable {
public:
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
    pid_t getId() const { return tid_; }
    const std::string& getName() const { return name_; }
    void join();
    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(std::string_view name);  
    static uint32_t GetStartTime();
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