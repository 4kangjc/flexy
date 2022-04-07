#include <flexy/flexy.h>
#include <variant>
#include "flexy/util/memory.h"


using namespace flexy;

static auto g_logger = FLEXY_LOG_ROOT();

// void _start() {

// }


namespace std {

// template <>
// struct is_invocable<const flexy::detail::__task> : std::false_type {};

// template <>
// struct is_invocable<flexy::detail::__task&> : std::false_type {};

// template <class _task>
// struct is_invocable<_task> {
//     const bool value = flexy::detail::is_task<_task>;
// };

} // namespace std

int main() {
    // Fiber::GetThis();
    // Fiber::ptr f = make_shared<Fiber>(0, [](){
    //     FLEXY_LOG_DEBUG(g_logger) << "Hello fiber!";
    //     Fiber::Yield();
    //     // FLEXY_LOG_DEBUG(g_logger);
    // });
    // f->resume();

    // Fiber::ptr f2(make_shared<Fiber>(0, [](){
    //     FLEXY_LOG_DEBUG(g_logger) << "Hello fiber2!";
    // }));
    // f2->resume();
    // f->resume();


    // detail::__task tk([]() {});
    // detail::__task([]() {
    //     FLEXY_LOG_DEBUG(g_logger) << "Hello task";
    // })();
    // // t.swap(tk);
    // std::function<void()>([](){
    //     FLEXY_LOG_DEBUG(g_logger) << "Hello function";
    // })();
    // t();
    // std::is_invocable_v<decltype(t_r)>
    // const auto&& t3 = std::move(t);
    // detail::__task t2(std::move(t3));
    // detail::__task t2(std::move(t));
    // std::function<void()> cb;

    Scheduler iom;
    iom.start();
    go [](){
        FLEXY_LOG_DEBUG(g_logger) << "HELLO GO";
    };
    iom.stop();

}