#include "fiber.h"
#include "allocator.h"
#include "flexy/util/macro.h"
#include "flexy/util/config.h"
#include "scheduler.h"
#include <atomic>

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};            // 分配协程id
static std::atomic<uint64_t> s_fiber_count {0};         // 记录正在运行的协程数量

static thread_local Fiber* t_fiber = nullptr;               // run fiber
static thread_local Fiber::ptr t_threadFiber = nullptr;     // main fiber

static auto g_fiber_stack_size = Config::Lookup("fiber.stack_size", 128u * 1024u, "fiber stack size");

using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
    state_ = EXEC;
    SetThis(this);

    ++s_fiber_count;
    FLEXY_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

// Fiber::Fiber(std::function<void()>&& cb, size_t stacksize)
//         : id_(++s_fiber_id), cb_(std::move(cb)) {
//     ++s_fiber_count;
//     stacksize_ = stacksize ? stacksize : g_fiber_stack_size->getValue();

//     stack_ = StackAllocator::Alloc(stacksize_);

//     ctx_ = make_fcontext((char*)stack_ + stacksize_, stacksize_, &Fiber::MainFunc);
//     FLEXY_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << id_;
// }

Fiber::Fiber(size_t stacksize, detail::__task&& task) 
    : id_(++s_fiber_id), cb_(std::move(task))
{
    ++s_fiber_count;
    stacksize_ = stacksize ? stacksize : g_fiber_stack_size->getValue();

    stack_ = StackAllocator::Alloc(stacksize_);

    ctx_ = make_fcontext((char*)stack_ + stacksize_, stacksize_, &Fiber::MainFunc);

    FLEXY_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << id_;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if (stack_) {                                          // 子协程
        FLEXY_ASSERT(state_ != EXEC);
        StackAllocator::Dealloc(stack_, stacksize_);
    } else {
        FLEXY_ASSERT2(state_ == EXEC, "m_state = " << state_);   // 主协程一定在运行

        Fiber* cur = t_fiber;
        FLEXY_ASSERT(cur == this);                                // 主协程最后析构，此时运行的协程一定是主协程
        SetThis(nullptr);
    }
    FLEXY_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << id_ << " total = " << s_fiber_count;
}


void Fiber::reset(std::function<void()>&& cb) {
    FLEXY_ASSERT(stack_);                            // 子协程才能重置
    FLEXY_ASSERT(state_ != EXEC);                    // 没有在运行
    cb_ = std::move(cb);

    ctx_ = make_fcontext((char*)stack_ + stacksize_, stacksize_, &Fiber::MainFunc);
    state_ = READY;
}

void Fiber::resume() {
    FLEXY_ASSERT(state_ == READY);
    SetThis(this);                         
    state_ = EXEC;
    
    t_threadFiber->state_ = READY;

    ctx_ = jump_fcontext(ctx_, nullptr).fctx;
}

transfer_t ontop_callback(transfer_t from) {
    Fiber* fiber = (Fiber*)(from.data);
    fiber->state_ = Fiber::READY;
    
    return from;
}

void Fiber::yield() {
    FLEXY_ASSERT(state_ != READY);
    
    SetThis(t_threadFiber.get());
    t_threadFiber->state_ = EXEC;
    // auto p = jump_fcontext(t_threadFiber->ctx_, nullptr);
    auto p = ontop_fcontext(t_threadFiber->ctx_, this, ontop_callback);
    t_threadFiber->ctx_ = p.fctx;
}

void Fiber::_M_return() const {
    FLEXY_ASSERT(state_ == Fiber::TERM);
    SetThis(t_threadFiber.get());
    t_threadFiber->state_ = EXEC;
    // t_threadFiber->ctx_ = jump_fcontext(t_threadFiber->ctx_, nullptr).fctx;
    jump_fcontext(t_threadFiber->ctx_, nullptr);
}

void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    FLEXY_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = std::move(main_fiber);
    return t_fiber->shared_from_this();
} 


uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}   

void Fiber::MainFunc(transfer_t t) {
    auto&& cur = GetThis();
    FLEXY_ASSERT(cur);

   
    t_threadFiber->ctx_ = t.fctx;

    try {
        cur->cb_();
        cur->cb_ = nullptr;
        cur->state_ = TERM;
    } catch (std::exception& ex) {
        cur->state_ = EXCEPT;
        FLEXY_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber id = " << cur->getId() 
            << std::endl << BacktraceToString();
    } catch (...) {
        cur->state_ = EXCEPT;  
        FLEXY_LOG_ERROR(g_logger) << "Fiber Except: "<< " fiber id = " << cur->getId() 
            << std::endl << BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur = nullptr;
    // raw_ptr->yield();
    raw_ptr->_M_return();

    FLEXY_ASSERT2(false, "never reach fiber id = " + std::to_string(raw_ptr->getId()));
}

uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

}