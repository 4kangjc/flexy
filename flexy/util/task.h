#pragma once

#include <functional>
#include <memory>

class __task {
public:
    template <typename F, typename... A>
    __task(F&& func, A&&... args) {
        using shared_type = decltype(
            std::bind(std::forward<F>(func), std::forward<A>(args)...));
        auto tkptr = std::make_shared<shared_type>(
            std::bind(std::forward<F>(func), std::forward<A>(args)...));
        cb_ = [tk = std::move(tkptr)](){ (*tk)(); };
    }
    template <typename F>
    __task(F&& func) : cb_(std::forward<F>(func)) { }
    __task() = default;
    __task(__task&& other) noexcept : cb_(std::move(other.cb_)) { }
    __task(const __task& other) : cb_(other.cb_) {}
    ~__task() = default;
    __task& operator=(const __task& cb) { cb_ = cb.cb_; return *this; }
    __task& operator=(__task&& cb) { cb_ = std::move(cb.cb_); return *this; }
    void operator()() { return cb_(); }
    operator bool() { return cb_ != nullptr; }
    operator std::function<void()>() const { return cb_; }
    void swap(__task& cb) noexcept { cb_.swap(cb.cb_); }
private:
    std::function<void()> cb_;
};