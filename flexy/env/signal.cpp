#include "signal.h"
#include "flexy/schedule/iomanager.h"
#include <cstring>
// #include <signal.h>          // 放到头文件中

namespace flexy {

// std::function<void()> handlers[65];
__task handlers[65];

static void signal_handler(int sig) {
    auto iom = Scheduler::GetThis();
    if (iom) {
        iom->async(handlers[sig]);
    } else {
        handlers[sig]();
    }
}

void Signal::signal(int sig, __task&& handler) {
    // handlers[sig] = std::move(handler.get());
    handlers[sig] = std::move(handler);
    ::signal(sig, signal_handler);
    // struct sigaction sa;
    // memset(&sa, '\0', sizeof(sa));
    // sa.sa_handler = signal_handler;
    // sa.sa_flags  |= SA_RESTART;
    // sigfillset(&sa.sa_mask);
    // sigaction(sig, &sa, nullptr);
}

}