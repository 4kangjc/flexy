#pragma once

#include "flexy/util/memory.h"
#include "flexy/util/task.h"

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

    template <typename _First, typename... _Args>
    friend std::shared_ptr<Fiber> fiber_make_shared(_First&& __first, _Args&&... __args);

    enum State {
        READY,          // 就绪状态
        EXEC,           // 执行状态
        TERM,           // 结束状态
        EXCEPT,         // 异常状态
    };

private:
    explicit Fiber(std::size_t stacksize, detail::__task&& cb); // 子协程构造函数
    explicit Fiber();                                           // 主协程构造函数
public:
    ~Fiber();

    void reset(std::function<void()>&& cb);                      // 重置协程函数 并重置协程状态
    void reset(const std::function<void()>& cb) { return reset(std::function<void()>(cb)); }
    void yield();                                               // 让出执行权
    void yield_callback(std::function<void()>&& cb);            // 让出执行权后回调一个函数
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
    void _M_return() const;                                     // 协程返回
private:
    uint64_t id_ = 0;              // 协程id
    uint32_t stacksize_ = 0;       // 协程栈大小
    State state_ = READY;          // 协程状态
    fcontext_t ctx_;               // 协程上下文
    detail::__task cb_;            // 协程执行函数
    char stack_[];                 // 协程栈首指针
};


template <typename _First, typename... _Args>
std::shared_ptr<Fiber> fiber_make_shared(_First&& __first, _Args&&... __args) {

extern Fiber* MallocFiber(size_t& stacksize);
extern std::shared_ptr<Fiber> FreeFiber(Fiber* fiber);

    if constexpr (std::is_integral_v<_First>) {
        size_t stacksize = __first > 0 ? __first : 0;
        Fiber* fiber = MallocFiber(stacksize);
        new (fiber) Fiber(stacksize, detail::__task(std::forward<_Args>(__args)...));

        return FreeFiber(fiber);
    } else {
        size_t stacksize = 0;
        Fiber* fiber = MallocFiber(stacksize);
        new (fiber) Fiber(stacksize, detail::__task(std::forward<_First>(__first), std::forward<_Args>(__args)...));

        return FreeFiber(fiber);
    }
}

template <typename _Fiber, typename... _Args>
std::enable_if_t<std::is_same_v<_Fiber, Fiber>, Fiber::ptr> make_shared(_Args&&... __args) {
    return fiber_make_shared(std::forward<_Args>(__args)...);
}

namespace detail {

template <class _Tp, class = void>
struct is_fiber_ptr {
    const static bool value = false;
};

template <>
struct is_fiber_ptr<Fiber::ptr> {
    const static bool value = true;
};

}

template <class _Tp>
constexpr bool is_fiber_ptr_v = detail::is_fiber_ptr<std::decay_t<_Tp>>::value;

}