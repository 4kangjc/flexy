#include <flexy/schedule/iomanager.h>
#include <flexy/schedule/mutex.h>
#include <flexy/util/log.h>

static auto& g_logger = FLEXY_LOG_ROOT();

int sum;

flexy::fiber::mutex mutex;
// flexy::NullMutex mutex;
// flexy::mutex mutex;

void add() {
    LOCK_GUARD(mutex);
    for (int i = 0; i < 1000000; ++i) {
        ++sum;
    }
    FLEXY_LOG_DEBUG(g_logger) << "add finish!";
}

void diff() {
    LOCK_GUARD(mutex);
    for (int i = 0; i < 500000; ++i) {
        --sum;
    }
    FLEXY_LOG_DEBUG(g_logger) << "diff finish!";
}

int main() {
    flexy::Scheduler iom(2);
    iom.start();
    iom.async(add);
    iom.async(diff);
    iom.stop();
    FLEXY_LOG_DEBUG(g_logger) << "sum = " << sum;
}