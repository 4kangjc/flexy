#pragma once

#include <map>
#include "flexy/util/log.h"
#include "flexy/util/noncopyable.h"
#include "iomanager.h"
#include "semaphore.h"

namespace flexy {

class WorkerGroup : noncopyable,
                    public std::enable_shared_from_this<WorkerGroup> {
public:
    using ptr = std::shared_ptr<WorkerGroup>;
    static WorkerGroup::ptr Create(uint32_t batch_size,
                                   Scheduler* s = Scheduler::GetThis()) {
        return std::make_shared<WorkerGroup>(batch_size, s);
    }

    WorkerGroup(uint32_t batch_size, Scheduler* s = Scheduler::GetThis())
        : batchSize_(batch_size),
          finish_(false),
          scheduler_(s),
          sem_(batch_size) {}

    ~WorkerGroup() { waitAll(); }

    template <class... _Args,
              typename = std::enable_if_t<std::is_invocable_v<_Args&&...>>>
    void schedule(_Args&&... args) {
        static_assert(sizeof...(args) > 0, "args must > 0");
        sem_.wait();
        // scheduler_->async([self = shared_from_this()](auto&&... args) {
        //     // std::invoke(std::forward<decltype(args)>(args)...);      //
        //     functor 失败
        //     detail::__task(std::forward<decltype(args)>(args)...)();
        //     self->sem_.post();
        // }, std::forward<_Args>(args)...);
        scheduler_->async(
            [self = shared_from_this(),
             task = detail::__task(std::forward<_Args>(args)...)]() {
                task();
                self->sem_.post();
            });
    }

    void waitAll() {
        if (!finish_) {
            finish_ = true;
            for (uint32_t i = 0; i < batchSize_; ++i) {
                sem_.wait();
            }
        }
    }

private:
    template <class Fn, class... Args>
    void doWork(Fn&& fn, Args&&... args) {
        fn(std::forward<Args>(args)...);
        sem_.post();
    }

private:
    uint32_t batchSize_;
    bool finish_;
    Scheduler* scheduler_;
    fiber::Semaphore sem_;
};

class WorkerManager {
public:
    template <class T>
    using ptr = std::shared_ptr<T>;

    WorkerManager() : stop_(false) {}
    void add(const ptr<Scheduler>& s);
    ptr<Scheduler> get(const std::string& name);
    ptr<IOManager> getAsIOManager(const std::string& name);

    template <class... FnAndArgs>
    void schedule(const std::string& name, FnAndArgs&&... args) {
        auto s = get(name);
        if (s) {
            s->async(std::forward<FnAndArgs>(args)...);
        } else {
            static auto s_logger = FLEXY_LOG_NAME("system");
            FLEXY_LOG_ERROR(s_logger)
                << "scheduler name = " << name << " not exists";
        }
    }

    template <class Iter>
    void schedule(const std::string& name, Iter&& begin, Iter&& end) {
        auto s = get(name);
        if (s) {
            s->async(std::forward<Iter>(begin), std::forward<Iter>(end));
        } else {
            static auto s_logger = FLEXY_LOG_NAME("system");
            FLEXY_LOG_ERROR(s_logger)
                << "scheduler name = " << name << " not exists";
        }
    }

    bool init();
    bool init(
        const std::map<std::string, std::map<std::string, std::string>>& v);
    void stop();

    bool isStoped() const { return stop_; }
    std::ostream& dump(std::ostream& os);

    uint32_t getCount();

private:
    std::map<std::string, std::vector<ptr<Scheduler>>> datas_;
    bool stop_;
};

using WorkerMgr = Singleton<WorkerManager>;

}  // namespace flexy
