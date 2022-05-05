#pragma once


#include <new>
#include <functional>
#include <utility>

#include "likely.h"
#include "align.h"

namespace flexy {

template <class R>
class Function;

#if defined(__GNUC__) || defined(__clang__) 
template <class R, class... Args, bool kNoexcept>
class alignas(max_align_v) Function<R(Args...) noexcept(kNoexcept)> {
#else
template <class R, class... Args>
class alignas(max_align_v) Function<R(Args...)> {
    static constexpr bool kNoexcept = false;
#endif

public:
    template <class T>
    static constexpr bool implictly_convertible_v = 
        std::is_invocable_r_v<R, T, Args...> && 
        !std::is_same_v<std::decay_t<T>, Function> &&
        (!kNoexcept || std::is_nothrow_invocable_v<T, Args...>);

    constexpr Function() = default;

    Function(std::nullptr_t) {}

    template <class T, class = std::enable_if_t<implictly_convertible_v<T>>>
    Function(T&& action) {
        if constexpr (sizeof(std::decay_t<T>) <= kMaximumOptimizableSize) {
            ops_ = ErasedCopySmall(&object_, std::forward<T>(action));
        } else {
            ops_ = ErasedCopyLarge(&object_, std::forward<T>(action));
        }
    }

    template <class T, class = std::enable_if_t<implictly_convertible_v<T>>>
    Function& operator=(T&& action) {
        this->~Function();
        new (this) Function(std::forward<T>(action));
        return *this;
    }

    Function& operator=(Function&& function) noexcept {
        if (FLEXY_LIKELY(&function != this)) {
            this->~Function();
            new (this) Function(std::move(function));
        }
        return *this;
    }

    Function& operator=(std::nullptr_t) {
        if (auto ops = std::exchange(ops_, nullptr)) {
            ops->destroyer(&object_);
        }
        return *this;
    }

    Function(Function&& function) noexcept {
        ops_ = std::exchange(function.ops_, nullptr);
        if (ops_) {
            ops_->relocator(&object_, &function.object_);
        }
    }

    ~Function() {
        if (ops_) {
            ops_->destroyer(&object_);
        }
    }


    R operator()(Args... args) const noexcept(kNoexcept) {
        return ops_->invoker(&object_, std::forward<Args>(args)...);
    }

    constexpr explicit operator bool() const { return !!ops_; }

    void swap(Function& __x) {
        std::swap(object_, __x.object_);
        std::swap(ops_, __x.ops_);
    }
private:
    static constexpr std::size_t kMaximumOptimizableSize = 3 * sizeof(void*);

    struct TypeOps {
        using Invoker = R (*) (void* object, Args&&... args);
        using Relocator = void(*)(void* to, void* from);
        using Destroyer = void(*)(void* object);

        Invoker invoker;
        Relocator relocator;
        Destroyer destroyer;
    };

    template <class T>
    static R Invoke(T&& object, Args&&... args) {
        if constexpr (std::is_void_v<R>) {
            std::invoke(std::forward<T>(object), std::forward<Args>(args)...);
        } else {
            return std::invoke(std::forward<T>(object), std::forward<Args>(args)...);
        }
    }

    template <class T>
    const TypeOps* ErasedCopySmall(void* buffer, T&& object) {
        using Decayed = std::decay_t<T>;

        static constexpr TypeOps ops = {
            /* invoker */ 
            [](void* object, Args&&... args) -> R {
                return Invoke(*static_cast<Decayed*>(object), std::forward<Args>(args)...);
            },
            /* relocator */ 
            [](void* to, void* from) {
                new (to) Decayed(std::move(*static_cast<Decayed*>(from)));
                static_cast<Decayed*>(from)->~Decayed();
            },
            /* destroyer */
            [](void* object) {
                static_cast<Decayed*>(object)->~Decayed();
            }
        };

        new (buffer) Decayed(std::forward<T>(object));
        return &ops;
    }


    template <class T>
    const TypeOps* ErasedCopyLarge(void* buffer, T&& object) {
        using Decayed = std::decay_t<T>;
        using Stored = Decayed*;

        static constexpr TypeOps ops = {
            // invoker
            [](void* object, Args&&... args) -> R {
                return Invoke(**static_cast<Stored*>(object), std::forward<Args>(args)...);
            },
            // relocator
            [](void* to, void* from) {
                new (to) Stored(*static_cast<Stored*>(from));
            },
            // destroyer
            [](void* object) {
                delete *static_cast<Stored*>(object);
            }
        };

        new (buffer) Stored(new Decayed(std::forward<T>(object)));
        return &ops;
    }

    mutable std::aligned_storage_t<kMaximumOptimizableSize, 1> object_;
    const TypeOps* ops_ = nullptr;
};

namespace detail {

template <class TSignature>
struct FunctionTypeDeducer;

template <class T>
using FunctionSignature = typename FunctionTypeDeducer<T>::Type;

}

#if defined(__GUNC__) || defined(__clang__)
template <class R, class... Args, bool kNoexcept>
Function(R(*)(Args...) noexcept(kNoexcept)) -> Function<R(Args...) noexcept(kNoexcept)>;
#else
template <class R, class... Args>
Function(R(*)(Args...)) -> Function<R(Args...)>;
#endif

template <class F, class Signature = detail::FunctionSignature<decltype(&F::operator())>>
Function(F) -> Function<Signature>;

// Comparison to `nullptr`.
template <class T>
bool operator==(const Function<T>& f, std::nullptr_t) {
  return !f;
}

template <class T>
bool operator==(std::nullptr_t, const Function<T>& f) {
  return !f;
}

template <class T>
bool operator!=(const Function<T>& f, std::nullptr_t) {
  return !(f == nullptr);
}

template <class T>
bool operator!=(std::nullptr_t, const Function<T>& f) {
  return !(nullptr == f);
}

namespace detail {

#if defined(__GNUC__) || defined(__clang__)

template <class R, class Class, class... Args, bool kNoexcept>
struct FunctionTypeDeducer<R (Class::*)(Args...) noexcept(kNoexcept)> {
  using Type = R(Args...) noexcept(kNoexcept);
};

template <class R, class Class, class... Args, bool kNoexcept>
struct FunctionTypeDeducer<R (Class::*)(Args...)& noexcept(kNoexcept)> {
  using Type = R(Args...) noexcept(kNoexcept);
};

template <class R, class Class, class... Args, bool kNoexcept>
struct FunctionTypeDeducer<R (Class::*)(Args...) const noexcept(kNoexcept)> {
  using Type = R(Args...) noexcept(kNoexcept);
};

template <class R, class Class, class... Args, bool kNoexcept>
struct FunctionTypeDeducer<R (Class::*)(Args...) const& noexcept(kNoexcept)> {
  using Type = R(Args...) noexcept(kNoexcept);
};

#else

template <class R, class Class, class... Args>
struct FunctionTypeDeducer<R (Class::*)(Args...)> {
  using Type = R(Args...);
};

template <class R, class Class, class... Args>
struct FunctionTypeDeducer<R (Class::*)(Args...)&> {
  using Type = R(Args...);
};

template <class R, class Class, class... Args>
struct FunctionTypeDeducer<R (Class::*)(Args...) const> {
  using Type = R(Args...);
};

template <class R, class Class, class... Args>
struct FunctionTypeDeducer<R (Class::*)(Args...) const&> {
  using Type = R(Args...);
};

#endif

}

}   // namespace flexy