#include "thread.h"
#include "flexy/util/util.h"

namespace flexy {

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_name = "UNKOWN";
static thread_local uint64_t t_start = 0;                  // 线程开始运行的时间 毫秒数


Thread::~Thread() {
    if (thread_) {
        pthread_detach(thread_);
    }
}

void Thread::join() {
    if (thread_) {
        pthread_join(thread_, nullptr);
        thread_ = 0;
    }
}

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_name;
}

uint32_t Thread::GetStartTime() {
    return t_start;
}

void Thread::SetName(std::string_view name) {
    if (name.empty()) {
        return;
    }
    if (t_thread) {
        t_thread->name_ = name;
    }
    t_name = name;
}

void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_name = thread->name_;
    thread->tid_ = GetThreadId();
    pthread_setname_np(pthread_self(), thread->name_.substr(0, 15).c_str());

    thread->semapthore_.post();             // 确保Thread创建时就已经运行起来
    t_start = GetTimeMs();
    thread->cb_();
    return nullptr;
}

}