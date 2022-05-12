#pragma once
#include <memory>
#include <set>
#include "flexy/util/task.h"
#include "flexy/thread/mutex.h"
namespace flexy {

class TimerManager;
class Timer : public std::enable_shared_from_this<Timer> {
friend class TimerManager;
public:
    using ptr = std::shared_ptr<Timer>;
    // 取消定时器
    bool cancel();
    // 刷新设置定时器的执行时间
    bool refresh();
    // 重置定时器时间 from_now 是否从当前时间开始计算
    bool reset(uint64_t ms, bool from_now);
private:
    Timer(uint64_t ms, detail::__task&& cb, bool recurring, TimerManager* manager);
    Timer(uint64_t next);
private:
    bool recurring_;                                // 是否为循环定时器
    uint64_t ms_;                                   // 执行周期
    uint64_t next_;                                 // 精确的执行时间
    detail::__task cb_;                             // 回调函数
    TimerManager* manager_ = nullptr;               // 定时器所属定时器管理者
private:
    struct Comparator {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

class TimerManager {
friend class Timer;
public:
    TimerManager();
    virtual ~TimerManager() { }
    // 添加定时器
    template <typename... Args>
    Timer::ptr addTimer(uint64_t ms, Args&&... args) {
        Timer::ptr timer(new Timer(ms, detail::__task(std::forward<Args>(args)...), false, this));
        WRITELOCK2(mutex_);
        addTimer(timer, lk);
        return timer;
    }
    // 添加循环定时器
    template <typename... Args>
    Timer::ptr addRecTimer(uint64_t ms, Args&&... args) {
        Timer::ptr timer(new Timer(ms, detail::__task(std::forward<Args>(args)...), true, this));
        WRITELOCK2(mutex_);
        addTimer(timer, lk);
        return timer;
    }
    // 添加条件定时器
    template <typename... Args>
    Timer::ptr addCondtionTimer(uint64_t ms, std::weak_ptr<void()> weak_cond, Args&&... args) {
        return addTimer(ms, OnTimer, weak_cond, detail::__task(std::forward<Args>(args)...));
    }
    // 添加循环条件定时器
    template <typename... Args>
    Timer::ptr addRecCondtionTimer(uint64_t ms, std::weak_ptr<void()> weak_cond, Args&&... args) {
        return addRecTimer(ms, OnTimer, weak_cond, detail::__task(std::forward<Args>(args)...));
    }
    // 获取下一个要执行的定时器任务的时间
    uint64_t getNextTimer();
    // 获取已到期的定时器需要执行回调函数的列表
    std::vector<detail::__task> listExpiriedTimer();
    // 是否有定时器
    bool hasTimer() const;
protected:
    // 当有新的定时器插入到定时器的首部,执行该函数
    [[deprecated]] virtual void onTimerInsertedAtFront() = 0;
    // 注册函数
    template <typename... Args>
    void onRefreshNearest(Args&&... args) { refreshNearest_ = detail::__task(std::forward<Args>(args)...); }
    // 将定时器添加到timers_中
    void addTimer(Timer::ptr& val, unique_lock<mutex>& lock);
    // 将定时器添加到timers_中
    void addTimer(Timer::ptr&& val, unique_lock<mutex>& lock);
    // 检测服务器时间是否被调后了
    bool detectClockRollover(uint64_t now_ms);
private:
    mutable mutex mutex_;                                       // 锁
    std::set<Timer::ptr, Timer::Comparator> timers_;            // 定时器集合
    bool tickled_ = false;                                      // 是否触发onTimerInsertedAtFront
    uint64_t previouseTime_;                                    // 上次执行时间
protected:
    detail::__task refreshNearest_;                             // 当有新的定时器插入到定时器的首部,执行该函数
private:
    static void OnTimer(std::weak_ptr<void()> weak_cond, detail::__task&& cb);
};

} // namespace flexy