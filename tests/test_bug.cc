#include <flexy/flexy.h>
#include <variant>


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

int main(int argc, char** argv) {
    Fiber::GetThis();
    Fiber::ptr f = fiber_make_shared([](int argc, char** argv){
        FLEXY_LOG_DEBUG(g_logger) << "Hello fiber!";
        Fiber::Yield();
        for (int i = 1; i < argc; ++i) {
            FLEXY_LOG_DEBUG(g_logger) << argv[i];
        }
    }, argc, argv);
    f->resume();

    Fiber::ptr f2(make_shared<Fiber>(0, [](){
        FLEXY_LOG_DEBUG(g_logger) << "Hello fiber2!";
    }));
    f2->resume();
    f->resume();
    // std::cout << sizeof(std::enable_shared_from_this<Fiber>);

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

    // Scheduler iom;
    // iom.start();
    // go [](){
    //     FLEXY_LOG_DEBUG(g_logger) << "HELLO GO";
    // };
    // iom.stop();

}