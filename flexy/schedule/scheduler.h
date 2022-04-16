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
    // 放回协程调第器的名称
    auto& getName() const { return name_; }
    // 返回当前协程调度器
    static Scheduler* GetThis();
    // 返回当前协程调度器的调度协程
    static Fiber* GetMainFiber();
    // 开始协程调度器
    void start();
    // 停止协程调度器
    void stop();
    // 将任务加入到协程调度器中运行
    template <typename... FiberOrcb>
    void async(FiberOrcb&&... fc) {
        static_assert(sizeof...(fc) > 0);
        bool need_tickle = false;
        {
            LOCK_GUARD(mutex_);
            need_tickle = tasks_.empty();
            tasks_.emplace_back(std::forward<FiberOrcb>(fc)...);
        }
        if (need_tickle) {
            tickle_();
        }
    }
    // 将任务加入到协程调度器中优先运行
    template <typename... FiberOrcb>
    void async_first(FiberOrcb&&... fc) {
        static_assert(sizeof...(fc) > 0);
        bool need_tickle = false;
        {
            LOCK_GUARD(mutex_);
            need_tickle = tasks_.empty();
            tasks_.emplace_front(std::forward<FiberOrcb>(fc)...);
        }
        if (need_tickle) {
            tickle_();
        }
    }
    // 将 [begin, end)里的任务加入到协程调度器中运行
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
            tickle_();
        }
    }

    template <typename T>
    void operator-(T&& args) {
        return async(std::forward<T>(args));
    }

    template <typename... Args>
    void onIdle(Args&&... args) { idle_ = __task(std::forward<Args>(args)...); }

    template <typename... Args>
    void onTickle(Args&&... args) { tickle_ = __task(std::forward<Args>(args)...); }

    std::ostream& dump(std::ostream& os);
private:
    // 可执行对象
    struct Task {
        Fiber::ptr fiber = nullptr;
        __task cb = nullptr;

        template <typename... Args>
        Task(Args&&... args) : cb(std::forward<Args>(args)...) { }
        Task(const Fiber::ptr& fiber) : fiber(fiber) { }
        Task(Fiber::ptr& fiber) : fiber(fiber) { }
        Task(Fiber::ptr&& f) noexcept : fiber(std::move(f)) { }
        Task(const Fiber::ptr&& f) noexcept : fiber(std::move(f)) { }
        Task(__task* c) { cb.swap(*c); }
        Task(Fiber::ptr* f) { fiber.swap(*f); }
        void reset() {
            fiber = nullptr;
            cb = nullptr;
        }
        operator bool() { return fiber != nullptr || cb; }
    };

protected:
    // 通知调度器有任务了
    [[deprecated]] virtual void tickle();
    // 协程调度实体函数
    void run();
    // 返回是否可以停止
    virtual bool stopping();
    // 协程无任务调度时执行idle协程 
    [[deprecated]]  virtual void idle();
    // 设置当前协程调度器
    void setThis();
    // 是否有空闲线程
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
    __task idle_;                                                              // 协程无任务调度时执行idle协程
    __task tickle_;                                                            // 通知调度器有任务了
};

// 生命周期开始： 将当前协程切换到 target 协程调度器中执行 生命周期结束： 切换回最初的协程调度器中执行
class SchedulerSwitcher : public noncopyable {
public:
    SchedulerSwitcher(Scheduler* target);
    ~SchedulerSwitcher();
private:
    static void SwitchTo(Scheduler* target);
private:
    Scheduler* caller_;
};

} // namespace flexy

#define go *flexy::Scheduler::GetThis() - 
#define go_args(func, args...) flexy::Scheduler::GetThis()->async(func, ##args)