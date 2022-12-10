#include "flexy/fiber/fiber.h"
#include <atomic>
#include "flexy/fiber/allocator.h"
#include "flexy/schedule/scheduler.h"
#include "flexy/util/config.h"
#include "flexy/util/macro.h"

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};  // 分配协程id
static std::atomic<uint64_t> s_fiber_count{0};  // 记录正在运行的协程数量

static thread_local Fiber* t_current_fiber = nullptr;   // run fiber
static thread_local Fiber::ptr t_main_fiber = nullptr;  // main fiber

static auto g_fiber_stack_size =
    Config::Lookup("fiber.stack_size", 128u * 1024u, "fiber stack size");

using StackAllocator = MallocStackAllocator;

Fiber* MallocFiber(size_t& __first) {
    __first = __first ? __first : g_fiber_stack_size->getValue();

    Fiber* fiber = (Fiber*)StackAllocator::Alloc(sizeof(Fiber) + __first);
    // new (fiber) Fiber(__first, std::forward<_Args>(__args)...);
    return fiber;
}

std::shared_ptr<Fiber> FreeFiber(Fiber* fiber) {
    return std::shared_ptr<Fiber>(fiber, [](Fiber* fiber) {
        fiber->~Fiber();
        StackAllocator::Dealloc(fiber, 0);
    });
}

Fiber::Fiber() {
    state_ = EXEC;
    SetThis(this);

    ++s_fiber_count;
    FLEXY_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(size_t stacksize, detail::__task&& task)
    : id_(++s_fiber_id), cb_(std::move(task)) {
    ++s_fiber_count;
    // stacksize_ = stacksize ? stacksize : g_fiber_stack_size->getValue();
    stacksize_ = stacksize;

    // stack_ = StackAllocator::Alloc(stacksize_);

    ctx_ = _fl_make_fcontext((char*)stack_ + stacksize_, stacksize_,
                             &Fiber::MainFunc);

    FLEXY_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << id_;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if (stacksize_) {  // 子协程
        FLEXY_ASSERT(state_ != EXEC);
        // StackAllocator::Dealloc(stack_, stacksize_);
    } else {
        FLEXY_ASSERT2(state_ == EXEC,
                      "m_state = " << state_);  // 主协程一定在运行

        Fiber* cur = t_current_fiber;
        FLEXY_ASSERT(cur ==
                     this);  // 主协程最后析构，此时运行的协程一定是主协程
        SetThis(nullptr);
    }
    FLEXY_LOG_DEBUG(g_logger)
        << "Fiber::~Fiber id = " << id_ << " total = " << s_fiber_count;
}

void Fiber::reset(detail::__task&& cb) {
    FLEXY_ASSERT(stack_);          // 子协程才能重置
    FLEXY_ASSERT(state_ != EXEC);  // 没有在运行
    cb_ = std::move(cb);

    ctx_ = _fl_make_fcontext((char*)stack_ + stacksize_, stacksize_,
                             &Fiber::MainFunc);
    state_ = READY;
}

void Fiber::resume() {
    FLEXY_ASSERT(state_ == READY);
    auto caller = t_current_fiber;
    t_current_fiber = this;
    state_ = EXEC;

    auto [ctx, self] = _fl_jump_fcontext(ctx_, caller);

    ctx_ = ctx;
    static_cast<Fiber*>(self)->state_ = READY;
    t_current_fiber = caller;
}

void Fiber::yield() { t_main_fiber->resume(); }

void Fiber::yield_callback(detail::__task&& cb) {
    //     FLEXY_ASSERT(state_ == EXEC);
    //     // FLEXY_ASSERT()
    //     t_current_fiber = t_main_fiber.get();
    //     t_main_fiber->state_ = EXEC;

    //     // auto [ctx, self] = ontop_fcontext(t_main_fiber->ctx_, , );
}

void Fiber::SetThis(Fiber* f) { t_current_fiber = f; }

Fiber::ptr Fiber::GetThis() {
    if (t_current_fiber) {
        return t_current_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    FLEXY_ASSERT(t_current_fiber == main_fiber.get());
    t_main_fiber = std::move(main_fiber);
    return t_current_fiber->shared_from_this();
}

uint64_t Fiber::TotalFibers() { return s_fiber_count; }

void Fiber::MainFunc(transfer_t t) {
    auto&& cur = GetThis();
    FLEXY_ASSERT(cur);

    t_main_fiber->ctx_ = t.fctx;
    static_cast<Fiber*>(t.data)->state_ = READY;

    try {
        cur->cb_();
        cur->cb_ = nullptr;
        cur->state_ = TERM;
    } catch (std::exception& ex) {
        cur->state_ = EXCEPT;
        FLEXY_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                                  << " fiber id = " << cur->getId() << std::endl
                                  << BacktraceToString();
    } catch (...) {
        cur->state_ = EXCEPT;
        FLEXY_LOG_ERROR(g_logger) << "Fiber Except: "
                                  << " fiber id = " << cur->getId() << std::endl
                                  << BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur = nullptr;
    raw_ptr->yield();

    FLEXY_ASSERT2(false,
                  "never reach fiber id = " + std::to_string(raw_ptr->getId()));
}

uint64_t Fiber::GetFiberId() {
    if (t_current_fiber) {
        return t_current_fiber->getId();
    }
    return 0;
}

}  // namespace flexy