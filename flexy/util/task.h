#pragma once

#include <functional>
#include <memory>

namespace flexy::detail {

class __task_function;
class __task_virtual;
class __task_template;

template <class T, typename = void>
struct is_task {
    static constexpr bool value = false;
};

template <>
struct is_task<__task_function> {
    static constexpr bool value = true;
};

template <>
struct is_task<__task_virtual> {
    static constexpr bool value = true;
};

template <>
struct is_task<__task_template> {
    static constexpr bool value = true;
};



template <class _Tp>
constexpr bool is_task_v = is_task<std::decay_t<_Tp>>::value;


class __task_function : public std::function<void()> {
private:
    using callback_t = std::function<void()>;
public:
    template <typename _Fn, typename... _Args, 
              typename = std::enable_if_t<std::is_invocable_v<_Fn&&, _Args&&...>>>
    __task_function(_Fn&& func, _Args&&... args) 
        : callback_t(std::bind(std::forward<_Fn>(func), std::forward<_Args>(args)...))
    { }

    template <typename _Fn, typename = std::enable_if_t<std::is_invocable_v<_Fn&&> && !is_task_v<_Fn>>>
    __task_function(_Fn&& func) : callback_t(std::forward<_Fn>(func)) 
    { }

    __task_function(std::nullptr_t = nullptr) noexcept : callback_t() {}

};

template <typename _Tuple>
struct _Invoker {
    _Tuple _M_t;

    template <typename>
    struct __result {};

    template <typename _Fn, typename... _Args>
    struct __result<std::tuple<_Fn, _Args...>>
        : std::invoke_result<_Fn, _Args...>
    {};

    template <size_t... _Ind>
    typename __result<_Tuple>::type
    _M_invoke(std::_Index_tuple<_Ind...>) {
        return std::invoke(std::get<_Ind>(std::move(_M_t))...); 
    }

    typename __result<_Tuple>::type
    operator()() {
        using _Indices = typename std::_Build_index_tuple<std::tuple_size_v<_Tuple>>::__type;
        return _M_invoke(_Indices());

    }
};

template <typename... _Tp>
using Call_wrapper = _Invoker<std::tuple<typename std::decay_t<_Tp>...>>;

struct __task_virtual {
public:
    struct _Base {
        virtual ~_Base() = default;
        virtual void _M_run() = 0;
    };

    using _Base_ptr = std::shared_ptr<_Base>;

    template <typename _Fn, typename... _Args, 
              typename = std::enable_if_t<std::is_invocable_v<_Fn&&, _Args&&...>>>
    __task_virtual(_Fn&& func, _Args&&... args) 
    { 
        using Wrapper = Call_wrapper<_Fn, _Args...>;
        _base_ptr = std::make_shared<_Base_impl<Wrapper>>(std::forward<_Fn>(func), std::forward<_Args>(args)...);
    }

    template <typename _Fn, typename = std::enable_if_t<std::is_invocable_v<_Fn&&> 
                && !is_task_v<_Fn> && std::is_copy_constructible_v<_Fn&&>>>
    __task_virtual(_Fn&& func) {
        _base_ptr = std::make_shared<_Base_function>(std::forward<_Fn>(func));
    }

    __task_virtual(__task_virtual&& ) noexcept = default;
    __task_virtual(const __task_virtual& ) = default;
    __task_virtual(std::nullptr_t = nullptr) noexcept : _base_ptr() {}
    __task_virtual& operator=(__task_virtual&&) noexcept = default;
    __task_virtual& operator=(const __task_virtual&) = default;

    void operator()() const { _base_ptr->_M_run(); }
    explicit operator bool() { return _base_ptr.operator bool(); }

    void swap(__task_virtual& rhs) noexcept { _base_ptr.swap(rhs._base_ptr); }
private:
    template <typename _Callable>
    struct _Base_impl : public _Base {
        _Callable _M_func;

        template <typename... _Args>
        _Base_impl(_Args&&... __args) : _M_func{{std::forward<_Args>(__args)...}} 
        { }

        void _M_run() override { _M_func(); }
    };

    struct _Base_function : public _Base {
        std::function<void()> _M_func;

        template <typename _Fn>
        _Base_function(_Fn&& func) : _M_func(func)
        { }

        void _M_run() override { _M_func(); }
    };

private:
    _Base_ptr _base_ptr;
};

template <typename _Callable>
void __task_proxy(void* __vp) {
    auto invoker_ptr(static_cast<_Callable*>(__vp));
    if (invoker_ptr) {
        invoker_ptr->operator()();
    }
}

struct __task_template {
public:
    using proxy_ptr = std::function<void(void*)>;

    template <typename _Fn, typename... _Args, 
              typename = std::enable_if_t<std::is_invocable_v<_Fn&&, _Args&&...>>>
    __task_template(_Fn&& func, _Args&&... args) 
    {
        using Wrapper = Call_wrapper<_Fn, _Args...>;
        auto __vp = new Wrapper{{std::forward<_Fn>(func), std::forward<_Args>(args)...}};
        proxy_t = &__task_proxy<Wrapper>;
        vp.reset(__vp);
    }

    template <typename _Fn, typename = std::enable_if_t<std::is_invocable_v<_Fn&&> 
            && !is_task_v<_Fn> && std::is_copy_constructible_v<_Fn&&>>>
    __task_template(_Fn&& func) {
        proxy_t = [fn = std::forward<_Fn>(func)](void* null) mutable {
            fn();
        };
    }

    __task_template(__task_template&&) noexcept = default;
    __task_template(const __task_template&) = default;
    __task_template(std::nullptr_t = nullptr) {}
    __task_template& operator=(__task_template&&) noexcept = default;
    __task_template& operator=(const __task_template&) = default;

    void operator()() const { proxy_t(vp.get()); }
    explicit operator bool() { return proxy_t.operator bool(); }

    void swap(__task_template& rhs) noexcept {
        proxy_t.swap(rhs.proxy_t);
        vp.swap(rhs.vp);
    }
    
private:
    proxy_ptr proxy_t;
    std::shared_ptr<void> vp;
};


// using __task = __task_function;          // 缺点是 std::function must be CopyConstructible
// using __task = __task_virtual;              // 缺点是 使用了虚函数,可能会性能会稍微下降
using __task = __task_template;

} // namespace flexy::detail
