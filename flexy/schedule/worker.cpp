#include "worker.h"
#include "flexy/util/config.h"

namespace flexy {

static auto g_worker_config = Config::Lookup(
    "workers", std::map<std::string, std::map<std::string, std::string>>(),
    "worker config");

void WorkerManager::add(const ptr<Scheduler>& s) {
    datas_[s->getName()].push_back(s);
}

std::shared_ptr<Scheduler> WorkerManager::get(const std::string& name) {
    if (auto it = datas_.find(name); it != datas_.end()) {
        if (it->second.size() == 1) {
            return it->second[0];
        }
        return it->second[rand() % it->second.size()];
    }
    return nullptr;
}

std::shared_ptr<IOManager> WorkerManager::getAsIOManager(
    const std::string& name) {
    return std::dynamic_pointer_cast<IOManager>(get(name));
}

bool WorkerManager::init(
    const std::map<std::string, std::map<std::string, std::string>>& v) {
    for (auto& [name, map] : v) {
        int32_t thread_num = GetParamValue(map, "thread_num", 1);
        int32_t worker_num = GetParamValue(map, "worker_num", 1);

        datas_[name].emplace_back(new IOManager(thread_num, false, name));
        for (int32_t i = 1; i < worker_num; ++i) {
            datas_[name + "-" + std::to_string(i)].emplace_back(new IOManager(
                thread_num, false, name + "-" + std::to_string(i)));
        }
    }
    stop_ = datas_.empty();
    return true;
}

bool WorkerManager::init() {
    auto workers = g_worker_config->getValue();
    return init(workers);
}

void WorkerManager::stop() {
    if (stop_) {
        return;
    }
    for (auto& [name, sch_vec] : datas_) {
        for (auto& sche : sch_vec) {
            sche->async([]() {});
            sche->stop();
        }
    }
    datas_.clear();
    stop_ = true;
}

uint32_t WorkerManager::getCount() { return datas_.size(); }

std::ostream& WorkerManager::dump(std::ostream& os) {
    for (auto& [name, sch_vec] : datas_) {
        for (auto& sche : sch_vec) {
            sche->dump(os) << std::endl;
        }
    }
    return os;
}

}  // namespace flexy