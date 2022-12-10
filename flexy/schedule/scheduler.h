#pragma once

#include "flexy/fiber/fiber.h"
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
    // 开始协程调度器
    void start();
    // 停止协程调度器
    void stop();
    // 将任务加入到协程调度器中运行
    template <typename... _Args,
              typename = std::enable_if_t<std::is_invocable_v<_Args&&...>>>
    void async(_Args&&... __args) {
        static_assert(sizeof...(__args) > 0);
        bool need_tickle = false;
        {
            LOCK_GUARD(mutex_);
            need_tickle = tasks_.empty();
            tasks_.emplace_back(std::forward<_Args>(__args)...);
        }
        if (need_tickle) {
            tickle_();
        }
    }

    template <typename _Fiber,
              typename = std::enable_if_t<is_fiber_ptr_v<_Fiber>>>
    void async(_Fiber&& fiber) {
        bool need_tickle = false;
        {
            LOCK_GUARD(mutex_);
            need_tickle = tasks_.empty();
            tasks_.emplace_back(std::forward<_Fiber>(fiber));
        }
        if (need_tickle) {
            tickle_();
        }
    }
    // 将任务加入到协程调度器中优先运行
    template <typename... _Args>
    void async_first(_Args&&... __args) {
        static_assert(sizeof...(__args) > 0);
        bool need_tickle = false;
        {
            LOCK_GUARD(mutex_);
            need_tickle = tasks_.empty();
            tasks_.emplace_front(std::forward<_Args>(__args)...);
        }
        if (need_tickle) {
            tickle_();
        }
    }
    // 将 [begin, end)里的任务加入到协程调度器中运行
    template <typename Iterator>
    void async(Iterator begin, Iterator end) {
        bool need_tikle = false;
        {
            LOCK_GUARD(mutex_);
            need_tikle = tasks_.empty();
            while (begin != end) {
                // tasks_.emplace_back(std::forward<decltype(&*begin)>(&*begin));
                tasks_.push_back(std::move(*begin));
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
        detail::__task cb = nullptr;

        template <typename... _Args,
                  typename = std::enable_if_t<std::is_invocable_v<_Args&&...>>>
        Task(_Args&&... args) : cb(std::forward<_Args>(args)...) {}

        template <typename _Fiber,
                  typename = std::enable_if_t<is_fiber_ptr_v<_Fiber>>>
        Task(_Fiber&& fb) : fiber(std::forward<_Fiber>(fb)) {}

        Task(std::nullptr_t = nullptr) {}

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
    std::deque<Task> tasks_;  // 待执行的任务队列
    std::string name_;                                                         // 调度器名称        
protected:
    std::vector<int> threadIds_;                                               // 线程id数组
    size_t threadCount_                    = 0;                                // 线程数量
    std::atomic<size_t> activeThreadCount_ = {0};                              // 工作线程数量
    std::atomic<size_t> idleThreadCount_   = {0};                              // 空闲线程数量
    bool stopping_ = true;                                                     // 是否正在停止
    int rootThreadId_ = 0;                                                     // 主线程id(use_caller)
    detail::__task idle_;    // 协程无任务调度时执行idle协程
    detail::__task tickle_;  // 通知调度器有任务了
};

// 生命周期开始： 将当前协程切换到 target 协程调度器中执行 生命周期结束：
// 切换回最初的协程调度器中执行
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