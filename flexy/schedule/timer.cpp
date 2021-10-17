#include "timer.h"
#include "flexy/util/util.h"

namespace flexy {

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    if (lhs->next_ != rhs->next_) {
        return lhs->next_ < rhs->next_;
    }
    return lhs.get() < rhs.get(); 
}

Timer::Timer(uint64_t ms, __task&& cb, bool recurring, TimerManager* manager) 
    : recurring_(recurring), ms_(ms), cb_(std::move(cb)), manager_(manager) {
    next_ = GetTimeMs() + ms_;
}

Timer::Timer(uint64_t next) : next_(next) { }

bool Timer::cacel() {
    LOCK_GUARD(manager_->mutex_);
    if (cb_) {
        cb_ = nullptr;
        auto it = manager_->timers_.find(shared_from_this());
        manager_->timers_.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    LOCK_GUARD(manager_->mutex_);
    if (!cb_) {
        return false;
    }
    auto it = manager_->timers_.find(shared_from_this());
    if (it == manager_->timers_.end()) {
        return false;
    }
    manager_->timers_.erase(it);
    next_ = GetTimeMs() + ms_;
    manager_->timers_.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if (ms == ms_ && !from_now) {
        return true;
    }
    unique_lock<decltype(manager_->mutex_)> lock(manager_->mutex_);
    if (!cb_) {
        return false;
    }
    auto it = manager_->timers_.find(shared_from_this());
    if (it == manager_->timers_.end()) {
        return false;
    }
    manager_->timers_.erase(it);
    uint64_t start = from_now ? GetTimeMs() : next_ - ms_;
    ms_ = ms;
    next_ = ms_ + start;
    manager_->addTimer(shared_from_this(), lock);
    return true;
}

TimerManager::TimerManager() {
    previouseTime_ = GetTimeMs();
}

void TimerManager::addTimer(Timer::ptr& val, unique_lock<mutex>& lock) {
    return addTimer(Timer::ptr(val), lock);
}

void TimerManager::addTimer(Timer::ptr&& val, unique_lock<mutex>& lock) {
    auto it = timers_.insert(val).first;
    bool at_front = (it == timers_.begin() && !tickled_);           // 防止调用getNextTimer前多次调用onTimerInsetedAtFront
    if (at_front) {
        tickled_ = true;
    }
    lock.unlock();
    if (at_front) {
        onTimerInsertedAtFront();
    }
}

void TimerManager::OnTimer(std::weak_ptr<void()> weak_cond, __task&& cb) {
    auto tmp = weak_cond.lock();
    if (tmp) {
        cb();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollver = false;
    if (now_ms < previouseTime_ && now_ms < (previouseTime_ - 60 * 60 * 1000)) {
        rollver = true;
    }
    previouseTime_ = now_ms;
    return rollver;
}

bool TimerManager::hasTimer() const {
    LOCK_GUARD(mutex_);
    return !timers_.empty();
}

uint64_t TimerManager::getNextTimer() {
    LOCK_GUARD(mutex_);
    tickled_ = false;
    if (timers_.empty()) {
        return ~0ull;
    }
    auto& next = *timers_.begin();
    uint64_t now_ms = GetTimeMs();
    if (now_ms >= next->next_) {
        return 0;
    } else {
        return next->next_ - now_ms;
    }
}

std::vector<__task> TimerManager::listExpiriedTimer() {
    std::vector<__task> cbs;
    std::vector<Timer::ptr> expired;
    uint64_t now_ms = GetTimeMs();
    LOCK_GUARD(mutex_);
    if (timers_.empty()) {
        return cbs;
    }
    bool roller = detectClockRollover(now_ms);
    if (!roller && ((*timers_.begin())->next_ > now_ms)) {
        return cbs;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    auto it = roller ? timers_.end() : timers_.upper_bound(now_timer);
    expired.insert(expired.begin(), timers_.begin(), it);
    timers_.erase(timers_.begin(), it);

    for (auto& timer : expired) {
        if (timer->recurring_) {
            cbs.push_back(timer->cb_);
            timer->next_ = timer->ms_ + now_ms;
            timers_.insert(timer);
        } else {
            cbs.push_back(std::move(timer->cb_));
        }
    }  
    return cbs;
}

}