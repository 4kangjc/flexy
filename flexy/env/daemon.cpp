#include "daemon.h"
#include <sys/wait.h>
#include "flexy/util/config.h"

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");
static auto g_daemon_restart_interval = Config::Lookup<uint32_t>(
    "daemon.restart_interval", 5, "deamon restart interval");

std::ostream& ProcessInfo::dump(std::ostream& os) const {
    os << "[ProcessInfo parent_id = " << parent_id << " main_id = " << main_id
       << " parent_start_time = " << TimeToStr(parent_start_time)
       << " main_start_time = " << TimeToStr(main_start_time)
       << " restart_count = " << restart_count;
    return os;
}

std::string ProcessInfo::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

static int real_start(int argc, char** argv,
                      std::function<int(int argc, char** argv)> main_cb) {
    return main_cb(argc, argv);
}

static int real_daemon(int argc, char** argv,
                       std::function<int(int argc, char** argv)> main_cb) {
    [[maybe_unused]] auto res = daemon(1, 0);
    ProcessInfoMrg::GetInstance().parent_id = getpid();
    ProcessInfoMrg::GetInstance().parent_start_time = time(0);
    while (true) {
        pid_t pid = fork();
        if (pid == 0) {
            ProcessInfoMrg::GetInstance().main_id = getpid();
            ProcessInfoMrg::GetInstance().main_start_time = time(0);
            FLEXY_LOG_INFO(g_logger) << "Process start pid = " << getpid();
            return real_start(argc, argv, main_cb);
        } else if (pid < 0) {
            FLEXY_LOG_ERROR(g_logger)
                << "fork fail pid = " << pid << "errno = " << errno
                << " errstr = " << strerror(errno);
        } else {
            int status = 0;
            waitpid(pid, &status, 0);
            if (status) {
                FLEXY_LOG_ERROR(g_logger)
                    << "child crash pid = " << pid << " status = " << status;
            } else {
                FLEXY_LOG_INFO(g_logger) << "child finished pid = " << pid;
                break;
            }

            ProcessInfoMrg::GetInstance().restart_count++;
            sleep(g_daemon_restart_interval->getValue());
        }
    }
    return 0;
}

int start_daemon(int argc, char** argv,
                 std::function<int(int argc, char** argv)> main_cb,
                 bool is_daemon) {
    if (!is_daemon) {
        return real_start(argc, argv, main_cb);
    }
    return real_daemon(argc, argv, main_cb);
}
}  // namespace flexy