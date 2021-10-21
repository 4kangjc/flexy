#pragma once

#include "flexy/util/task.h"
#include <signal.h>

namespace flexy {

struct Signal {
public:
    template <typename... Args>
    static void signal(int sig, Args&&... args) {
        return signal(sig, __task(std::forward<Args>(args)...));
    }
private:
    static void signal(int sig, __task&& handler);
};

} // namespace flexy