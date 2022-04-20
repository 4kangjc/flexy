#include "fiber.h"
#include "iomanager.h"
#include "flexy/util/macro.h"
#include <chrono>

namespace flexy::this_fiber {

inline void yield() { return Fiber::GetThis()->yield(); }
inline uint64_t get_id() noexcept { return Fiber::GetThis()->getId(); }

template <typename... _Args, typename = std::enable_if_t<std::is_invocable_v<_Args&&...>>>
inline void yield_callback(_Args&&... __args) { 
    return Fiber::GetThis()->yield_callback(std::forward<_Args>(__args)...); 
}

template <class Rep, class Period>
inline void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
    auto iom = flexy::IOManager::GetThis();
    FLEXY_ASSERT(iom);
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration).count();
    iom->addTimer(ms, [iom, fiber = flexy::Fiber::GetThis()]() {
        iom->async(std::move(fiber));
    });
    yield();
}

template <class Clock, class Duration>
inline void sleep_until(const std::chrono::time_point<Clock, Duration>& sleep_time) {
    return sleep_for(sleep_time - Clock::now());
}

} // flexy this_fiber