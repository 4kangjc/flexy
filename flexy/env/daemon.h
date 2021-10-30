#pragma once

#include <unistd.h>
#include <stdint.h>
#include <functional>
#include "flexy/util/singleton.h"

namespace flexy {

struct ProcessInfo {
    pid_t parent_id;
    pid_t main_id;
    uint64_t parent_start_time;
    uint64_t main_start_time;
    uint32_t restart_count = 0;

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;
};

using ProcessInfoMrg = Singleton<ProcessInfo>;

int start_daemon(int argc, char** argv, std::function
            <int(int argc, char** argv)> main_cb, bool is_daemon);

}   // namespace flexy