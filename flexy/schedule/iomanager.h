#pragma once

#include "channel.h"
#include "timer.h"

namespace flexy {

class IOManager : public Scheduler, public TimerManager {
public:
    IOManager(size_t thread = 1, bool use_caller = true, std::string_view name = "");
    ~IOManager();
    template <typename... Args>
    bool addEvent(int fd, Event events, Args&&... args) {
        return onEvent(fd, events, __task(std::forward<Args>(args)...));
    }

    bool delEvent(int fd, Event event);
    bool delRead(int fd) { return delEvent(fd, Event::READ); }
    bool delWrite(int fd) { return delEvent(fd, Event::WRITE); }
    bool cancelEvent(int fd, Event event);
    bool cancelAll(int fd);
    bool cancelRead(int fd, Event event) { return cancelEvent(fd, Event::READ); }
    bool cancelWrite(int fd, Event event) { return cancelEvent(fd, Event::WRITE); }
    bool onEvent(int fd, Event event, __task&& cb = nullptr);

    template <typename... Args>
    bool onRead(int fd, Args&&... args) { 
        return onEvent(fd, Event::READ, __task(std::forward<Args>(args)...));
    }

    template <typename... Args>
    bool onWrite(int fd, Args&&... args) {
        return onEvent(fd, Event::WRITE, __task(std::forward<Args>(args)...)); 
    }

    static IOManager* GetThis();
protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;
    bool stopping(uint64_t& timeout);
    void channelResize(size_t size);
private:
    int epfd_;
    int tickleFds_[2];
    std::atomic<size_t> pendingEventCount_ = {0};
    mutex mutex_;
    std::vector<Channel*> channels_;
};

} // namespace flexy