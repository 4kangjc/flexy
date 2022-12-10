#pragma once

#include "flexy/schedule/semaphore.h"

namespace flexy {

template <typename _Tp>
class BlockingQueue {
public:
    using ptr = std::shared_ptr<BlockingQueue>;
    using data_type = std::shared_ptr<_Tp>;

    size_t push(const data_type& data) {
        size_t size = 0;
        {
            LOCK_GUARD(mutex_);
            datas_.push_back(data);
            size = datas_.size();
        }
        sem_.post();
        return size;
    }

    data_type pop() {
        sem_.wait();
        LOCK_GUARD(mutex_);
        auto v = std::move(datas_.front());
        datas_.pop_front();
        return v;
    }

    size_t size() {
        LOCK_GUARD(mutex_);
        return datas_.size();
    }

    bool empty() {
        LOCK_GUARD(mutex_);
        return datas_.empty();
    }

private:
    fiber::Semaphore sem_;
    mutable Spinlock mutex_;
    std::deque<data_type> datas_;
};

}  // namespace flexy