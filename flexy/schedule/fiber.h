#pragma once

#include <memory>
#include <functional>

typedef void* fcontext_t;

struct transfer_t {
    fcontext_t  fctx;
    void    *   data;
};

extern "C" {

transfer_t jump_fcontext( fcontext_t const to, void * vp);
fcontext_t make_fcontext( void * sp, std::size_t size, void (* fn)( transfer_t) );
transfer_t ontop_fcontext( fcontext_t const to, void * vp, transfer_t (*fn) (transfer_t));

}

namespace flexy {

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    using ptr = std::shared_ptr<Fiber>;
    friend class Scheduler;
    friend transfer_t ontop_callback(transfer_t);
    enum State {
        READY,          // 就绪状态
        EXEC,           // 执行状态
        TERM,           // 结束状态
        EXCEPT,         // 异常状态
    };
private:
    Fiber();                            // 主协程构造函数
public:
    Fiber(std::function<void()>&& cb, size_t stacksize = 0, bool use_caller = true);
    Fiber(const std::function<void()>& cb, size_t stacksize = 0, bool use_caller = true) 
        : Fiber(std::function<void()>(cb), stacksize, use_caller) { }
    ~Fiber();

    void reset(std::function<void()>&& cb);                      // 重置协程函数 并重置协程状态
    void reset(const std::function<void()>& cb) { return reset(std::function<void()>(cb)); }
    void yield();                                               // 让出执行权
    void resume();                                              // 进入协程

    uint64_t getId() const { return id_; }                      // 返回协程id
    State getState() const { return state_; }                   // 返回协程状态

    static void SetThis(Fiber*);                                // 设置当前协程
    static Fiber::ptr GetThis();                                // 返回当前协程
    static void Yield() { return GetThis()->yield(); }          // 让出当前协程的执行权
    static uint64_t TotalFibers();                              // 返回当前协程的总数量
    static void MainFunc(transfer_t);                           // 协程执行函数体
    static uint64_t GetFiberId();                               // 获得当前协程id
private:
    void _M_return();
private:
    uint64_t id_ = 0;              // 协程id
    uint32_t stacksize_ = 0;       // 协程栈大小
    State state_ = READY;          // 协程状态
    fcontext_t ctx_;               // 协程上下文
    void* stack_ = nullptr;        // 协程栈首指针
    std::function<void()> cb_;     // 协程执行函数
    bool use_scheduler_;           // 是否使用协程调度器 
};

}