#pragma once

#include <functional>

namespace flexy::detail {

extern "C" {

int printf(const char* fmt, ...);

}

class __task;

template <class T, typename = void>
struct is_task {
    static const bool value = false;
};

template <>
struct is_task<__task> {
    static const bool value = true;
};


template <class _Tp>
constexpr bool is_task_v = is_task<std::decay_t<_Tp>>::value;

class __task : public std::function<void()> {
private:
    using callback_t = std::function<void()>;
public:
    template <typename _Fn, typename... _Args, 
              typename = std::enable_if_t<std::is_invocable_v<_Fn&&, _Args&&...>>>
    __task(_Fn&& func, _Args&&... args) 
        : callback_t(std::bind(std::forward<_Fn>(func), std::forward<_Args>(args)...))
    { }

    template <typename _Fn, typename = std::enable_if_t<std::is_invocable_v<_Fn&&> && !is_task_v<_Fn>>>
    __task(_Fn&& func) : callback_t(std::forward<_Fn>(func)) {
        // printf("__task create!\n");
    }

    __task(std::nullptr_t = nullptr) noexcept : callback_t() {}

    // void operator()() const { callback_t(); }
    // void swap(__task& rhs) noexcept { callback_t::swap(dynamic_cast<callback_t&>(rhs)); }
};



} // namespace flexy::detail