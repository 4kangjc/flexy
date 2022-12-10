#include <flexy/flexy.h>

static auto g_logger = FLEXY_LOG_ROOT();

flexy::mutex m;
flexy::fiber::condition_variable cv;
std::string data;
bool ready = false;
bool processed = false;

void worker_thread() {
    FLEXY_LOG_DEBUG(g_logger) << "Worker thread begin...";
    // 等待直至 main() 发送数据
    flexy::unique_lock<decltype(m)> lk(m);
    cv.wait(lk, []() { return ready; });

    // 等待后，我们占有锁。
    FLEXY_LOG_DEBUG(g_logger) << "Worker thread is processing data";
    data += " after processing";

    // 发送数据回 main()
    processed = true;
    FLEXY_LOG_DEBUG(g_logger)
        << "Worker thread signals data processing completed";

    // 通知前完成手动解锁，以避免等待线程才被唤醒就阻塞
    lk.unlock();
    cv.notify_one();
}

int main1() {
    FLEXY_LOG_DEBUG(g_logger) << "main() thread begin...";
    // flexy::IOManager iom(2);
    // go worker_thread;

    data = "Example data";
    // 发送数据到 worker 线程
    {
        LOCK_GUARD(m);
        ready = true;
        FLEXY_LOG_DEBUG(g_logger) << "main() signals data ready for processing";
    }
    cv.notify_one();

    // 等候 worker
    {
        std::unique_lock<decltype(m)> lk(m);
        cv.wait(lk, [] { return processed; });
    }
    FLEXY_LOG_DEBUG(g_logger) << "Back in main(), data = " << data;

    return 0;
}

int main() {
    flexy::IOManager iom(2);
    go worker_thread;
    go main1;
}