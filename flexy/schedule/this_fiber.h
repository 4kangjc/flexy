#include "fiber.h"
#include "iomanager.h"
#include "flexy/util/macro.h"
#include <chrono>

namespace flexy::this_fiber {

inline void yield() { return Fiber::GetThis()->yield(); }
inline uint64_t get_id() noexcept { return Fiber::GetThis()->getId(); }

template <class Rep, class Period>
void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
    auto iom = flexy::IOManager::GetThis();
    FLEXY_ASSERT(iom);
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration).count();
    iom->addTimer(ms, [iom, fiber = flexy::Fiber::GetThis()]() {
        iom->async(std::move(fiber));
    });
    yield();
}

template <class Clock, class Duration>
void sleep_until(const std::chrono::time_point<Clock, Duration>& sleep_time) {
    return sleep_for(sleep_time - Clock::now());
}

} // flexy this_fiber