#include "config.h"
#include "file.h"
#include "flexy/env/env.h"
namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
    READLOCK(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    READLOCK(GetMutex());
    ConfigVarMap& m = GetDatas();
    for (auto& [key, ptr] : m) {
        cb(ptr);
    }
}

static void ListAllMember(const std::string& prefix, const YAML::Node& node, std::vector<std::pair<std::string, const YAML::Node>>& output) {
    if (prefix.find_first_not_of("abcdefghijkmlnopqrstuvwxyz._0123456789") != prefix.npos) {
        FLEXY_LOG_INFO(g_logger) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.emplace_back(prefix, node);
    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

static void ListAllMember(const std::string& prefix, const Json::Value& node, std::vector<std::pair<std::string, const Json::Value>>& output) {
    if (prefix.find_first_not_of("abcdefghijkmlnopqrstuvwxyz._0123456789") != prefix.npos) {
        FLEXY_LOG_INFO(g_logger) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.emplace_back(prefix, node);
    if (node.isObject()) {
        auto mem = node.getMemberNames();
        for (auto it = mem.begin(); it != mem.end(); ++it) {
            ListAllMember(prefix.empty() ? *it : prefix + "." + *it, node[*it], output);
        }
    }
}

void Config::LoadFromYaml(const YAML::Node& root) {
    std::vector<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);
    for (auto& [key, node] : all_nodes) {
        if (key.empty()) {
            continue;
        }
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        auto var = LookupBase(key);
        if (var) {
            if (node.IsScalar()) {
                var->fromString(node.Scalar());
            } else {
                std::stringstream ss;
                ss << node;
                var->fromString(ss.str());
            }
        }
    }
}

void Config::LoadFromJson(const Json::Value& root) {
    std::vector<std::pair<std::string, const Json::Value>> all_nodes;
    ListAllMember("", root, all_nodes);
    for (auto& [key, node] : all_nodes) {
        if (key.empty()) {
            continue;
        }
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        auto var = LookupBase(key);
        if (var) {
            std::stringstream ss;
            ss << node;
            var->fromString(ss.str());
        }
    }
}

static std::map<std::string, uint64_t> s_filelastmodtime;
static mutex s_mutex;

template <bool json>
void Config::LoadFromConDir(std::string_view path) {
    std::string absolute_path;
    if (auto& env = EnvMgr::GetInstance(); env.init_) {
        absolute_path = env.getAbsolutePath(path);
    } else {
        absolute_path = FS::AbsolutePath(path);
    }
    std::vector<std::string> files;
    if constexpr (!json) {
        FS::ListAllFile(files, absolute_path, ".yml");
    } else {
        FS::ListAllFile(files, absolute_path, ".json");
    }

    for (const auto& file : files) {
        auto t = FS::LastWriteTime(file);
        {
            LOCK_GUARD(s_mutex);
            if (s_filelastmodtime[file] == t) {
                continue;
            }
            s_filelastmodtime[file] = t;
        }
        try {
            if constexpr(!json) {
                YAML::Node root = YAML::LoadFile(file);
                LoadFromYaml(root);
            } else {
                static Json::Reader r;
                std::ifstream is(file);
                Json::Value root;
                r.parse(is, root);
                LoadFromJson(root);
            }
            FLEXY_LOG_INFO(g_logger) << "LoadConfFile file = " << file << " ok";
        } catch (...) {
            FLEXY_LOG_ERROR(g_logger) << "LoadConfFile file = " << file << " failed";
        }
    }
}

void Config::LoadFromConDir(std::string_view path) {
    LoadFromConDir<false>(path);
    LoadFromConDir<true>(path);
}

} // namespace flexy
