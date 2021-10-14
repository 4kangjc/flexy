#pragma once

#include "fiber.h"
#include "flexy/thread/mutex.h"
#include "flexy/thread/thread.h"

#include <string_view>
#include <deque>

namespace flexy {

class Scheduler {
public:
    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否使用当前调用线程
     * @param[in] name 协程调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, std::string_view name = "");
    virtual ~Scheduler();
    auto& getName() const { return name_; }
    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    void start();
    void stop();

    template <typename... FiberOrcb>
    void async(FiberOrcb&&... fc) {
        bool need_tickle = false;
        {
            LOCK_GUARD(mutex_);
            need_tickle = tasks_.empty();
            tasks_.emplace_back(std::forward<FiberOrcb>(fc)...);
        }
        if (need_tickle) {
            tickle();
        }
    }

    template <typename... FiberOrcb>
    void async_first(FiberOrcb&&... fc) {
                bool need_tickle = false;
        {
            LOCK_GUARD(mutex_);
            need_tickle = tasks_.empty();
            tasks_.emplace_front(std::forward<FiberOrcb>(fc)...);
        }
        if (need_tickle) {
            tickle();
        }
    }

    template <typename Iterator>
    void async(Iterator&& begin, Iterator&& end) {
        bool need_tikle = false;
        {
            LOCK_GUARD(mutex_);
            need_tikle = tasks_.empty();
            while (begin != end) {
                tasks_.emplace_back(std::forward<decltype(&*begin)>(&*begin));
                ++begin;
            }
        }
        if (need_tikle) {
            tickle();
        }
    }
private:
    // 可执行对象
    struct Task {
        Fiber::ptr fiber = nullptr;
        __task cb = nullptr;

        template <typename... Args>
        Task(Args&&... args) : cb(std::forward<Args>(args)...) { }
        Task(const Fiber::ptr& fiber) : fiber(fiber) { }
        Task(Fiber::ptr&& f) noexcept : fiber(std::move(f)) { }
        Task(__task* c) { cb.swap(*c); }
        Task(Fiber::ptr* f) { fiber.swap(*f); }
        void reset() {
            fiber = nullptr;
            cb = nullptr;
        }
        operator bool() { return fiber != nullptr || cb; }
    };

protected:
    virtual void tickle();
    void run();
    virtual bool stopping();
    virtual void idle();
    void setThis();
    bool hasIdleThreads() const { return idleThreadCount_ > 0; }
private:    
    mutable mutex mutex_;                                                      // Mutex       
    std::vector<Thread::ptr> threads_;                                         // 线程池                                    
    std::deque<Task> tasks_;                                                   // 待执行的任务队列 
    Fiber::ptr rootFiber_;                                                     // user_caller为true时有效，调度协程                                       
    std::string name_;                                                         // 调度器名称        
protected:
    std::vector<int> threadIds_;                                               // 线程id数组
    size_t threadCount_                    = 0;                                // 线程数量
    std::atomic<size_t> activeThreadCount_ = {0};                              // 工作线程数量
    std::atomic<size_t> idleThreadCount_   = {0};                              // 空闲线程数量
    bool stopping_ = true;                                                     // 是否正在停止
    int rootThreadId_ = 0;                                                     // 主线程id(use_caller)

};


} // namespace flexy