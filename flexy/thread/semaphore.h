#pragma once

#include "flexy/util/noncopyable.h"
#include <semaphore.h>
#include <stdint.h>
#include <stdexcept>

namespace flexy {

class Semaphore : noncopyable {
public:
    Semaphore(uint32_t count = 0) {
        if (sem_init(&semaphore_, 0, count)) {
            throw std::logic_error("sem_init error");
        }
    }
    ~Semaphore() {
        sem_destroy(&semaphore_);
    }
    void wait() {
        if (sem_wait(&semaphore_)) {
            throw std::logic_error("sem_wait error");
        }
    }
    void post() {
        if (sem_post(&semaphore_)) {
            throw std::logic_error("sem_post error");
        }
    }
private:
    Semaphore(Semaphore&& ) = delete;
private:
    sem_t semaphore_;
};

} // namespace flexy