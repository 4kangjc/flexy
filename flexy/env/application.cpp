#include "application.h"
#include "env.h"
#include "daemon.h"
#include "flexy/util/config.h"
#include "flexy/util/file.h"
#include "flexy/schedule/worker.h"

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

static auto g_server_work_path = Config::Lookup<std::string>("server.work_path", "/apps/work/flexy", "server work path");

static auto g_server_pid_file = Config::Lookup<std::string>("server.pid_file", "flexy.pid", "server pid file");

struct TcpServerConf {
    std::vector<std::string> address;
    int keepalive = 0;
    int timeout = 1000 * 2 * 60;
    std::string name;
    std::string type = "http";
    std::string accept_worker;
    std::string io_worker;
    std::string process_worker;

    bool isValid() const {
        return !address.empty();
    }

    bool operator==(const TcpServerConf& other) const {
        return name == other.name && keepalive == other.keepalive
         && timeout == other.timeout && address == other.address
         && type == other.type && accept_worker == other.accept_worker
         && io_worker == other.io_worker && process_worker == other.process_worker;
    }
};

template<>
struct LexicalCastYaml<std::string, TcpServerConf> {
    decltype(auto) operator() (const std::string& v) {
        YAML::Node node = YAML::Load(v);
        TcpServerConf conf;
        conf.name = node["name"].as<std::string>();
        conf.keepalive = node["keepalive"].as<int>();
        conf.timeout = node["timeout"].as<int>();
        conf.type = node["type"].as<std::string>();
        conf.accept_worker = node["accept_worker"].as<std::string>();
        conf.io_worker = node["io_worker"].as<std::string>();
        conf.process_worker = node["process_worker"].as<std::string>();
        if (node["address"].IsDefined()) {
            conf.address.reserve(node["address"].size());
            for (size_t i = 0; i < node["address"].size(); ++i) {
                conf.address.emplace_back(node["address"][i].as<std::string>());
            }
        }
        return conf; 
    }
    
};

template<>
struct LexicalCastYaml<TcpServerConf, std::string> {
    decltype(auto) operator() (const TcpServerConf& v) {
        YAML::Node node;
        node["name"] = v.name;
        node["keepalive"] = v.keepalive;
        node["timeout"] = v.timeout;
        node["type"] = v.type;
        node["accept_worker"] = v.accept_worker;
        node["io_worker"] = v.io_worker;
        node["process_worker"] = v.process_worker;
        for (const auto& s : v.address) {
            node["address"].push_back(s);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<>
struct LexicalCastJson<std::string, TcpServerConf> {
    decltype(auto) operator() (const std::string& v) {
        Json::Reader r;
        Json::Value node;
        r.parse(v, node);
        TcpServerConf conf;
        conf.name = node["name"].asString();
        conf.keepalive = node["keepalive"].asInt();
        conf.timeout = node["timeout"].asInt();
        conf.type = node["type"].asString();
        conf.accept_worker = node["accept_worker"].asString();
        conf.io_worker = node["io_worker"].asString();
        conf.process_worker = node["process_worker"].asString();
        if (node.isMember("address")) {
            int n = node["address"].size();
            conf.address.reserve(n);
            for (int i = 0; i < n; ++i) {
                conf.address.emplace_back(node["address"][i].asString());
            }
        }
        return conf;
    }
};

template<>
struct LexicalCastJson<TcpServerConf, std::string> {
    decltype(auto) operator() (const TcpServerConf& v) {
        Json::Value node;
        node["name"] = v.name;
        node["keepalive"] = v.keepalive;
        node["timeout"] = v.timeout;
        node["type"] = v.type;
        node["accept_worker"] = v.accept_worker;
        node["io_worker"] = v.io_worker;
        node["process_worker"] = v.process_worker;
        for (const auto& i : v.address) {
            node["address"].append(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


static auto g_tcp_server_conf = Config::Lookup("servers", std::vector<TcpServerConf>(), "tcp server config");   

Application::Application() {
    s_instance = this;
}

bool Application::init(int argc, char** argv) {
    argc_ = argc;
    argv_ = argv;
    EnvMgr::GetInstance().addHelp("s", "start with the terminal");
    EnvMgr::GetInstance().addHelp("d", "run as daemon");
    EnvMgr::GetInstance().addHelp("c", "conf path default ./conf");
    EnvMgr::GetInstance().addHelp("p", "print help");
    EnvMgr::GetInstance().addHelp("y", "load yaml config dir");
    EnvMgr::GetInstance().addHelp("j", "load json config dir");

    if (!EnvMgr::GetInstance().init(argc, argv)) {
        EnvMgr::GetInstance().printHelp();
        return false;
    }

    if (EnvMgr::GetInstance().has("p")) {
        EnvMgr::GetInstance().printHelp();
        return false;
    }

    int run_type = 0;
    if (EnvMgr::GetInstance().has("s")) {
        run_type = 1;
    } else if (EnvMgr::GetInstance().has("d")) {
        run_type = 2;
    }

    if (run_type == 0) {
        EnvMgr::GetInstance().printHelp();
        return false;
    }

    auto pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();
    if (FS::IsRunningPidfile(pidfile)) {
        FLEXY_LOG_ERROR(g_logger) << "server is running: " << pidfile;
        return false;
    }
    auto conf_path = EnvMgr::GetInstance().getAbsolutePath(EnvMgr::GetInstance().get("c", "conf"));
    FLEXY_LOG_INFO(g_logger) << "load conf path: " << conf_path;

    int load_type = 0;

    if (EnvMgr::GetInstance().has("y")) {
        load_type |= 1; 
    }
    if (EnvMgr::GetInstance().has("j")) {
        load_type |= 2;
    }

    switch (load_type) {
    case 0:
        Config::LoadFromConDir<false>(conf_path);
        break;
    case 1:
        Config::LoadFromConDir<false>(conf_path);
        break;
    case 2:
        Config::LoadFromConDir<true>(conf_path);
        break;
    case 3:
        Config::LoadFromConDir(conf_path); 
    default:
        break;
    }

    if (!FS::Mkdir(g_server_work_path->getValue())) {
        FLEXY_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
        << "] errno = " << errno << " errstr = " << strerror(errno);
    }
    
    return true;
}

bool Application::run() {
    bool is_daemon = EnvMgr::GetInstance().has("d");
    return start_daemon(argc_, argv_, [this](int argc, char** argv) {
        return main(argc, argv);
    }, is_daemon);
}

void Application::run_fiber() {
    FLEXY_LOG_DEBUG(g_logger) << "run fiber";
    WorkerMgr::GetInstance().init();

    auto tcp_confs = g_tcp_server_conf->getValue();
    for (auto& i : tcp_confs) {
        FLEXY_LOG_INFO(g_logger) << YamlToStr<TcpServerConf>()(i);
        std::vector<Address::ptr> address;
        for (auto& a : i.address) {
            size_t pos = a.find(":");
            if (pos == a.npos) {
                // FLEXY_LOG_ERROR(g_logger) << "invalid address: " << a;
                address.push_back(std::make_shared<UnixAddress>(a));
                continue;
            }
            auto addr = Address::LookupAny(a);
            if (addr) {
                address.push_back(addr);
                continue;
            }

            auto result = Address::GetInterfaceAddress(std::string(a.c_str(), pos));
            if (result) {
                for (auto& [x, y] : *result) {
                    auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x);
                    if (ipaddr) {
                        ipaddr->setPort(::atoi(a.c_str() + pos + 1));
                        address.push_back(ipaddr);
                    }
                }
            } else {
                FLEXY_LOG_ERROR(g_logger) << "invalid address: " << a;
                continue;
            }
        }

        IOManager* accept_worker = IOManager::GetThis();
        IOManager* io_worker = IOManager::GetThis();
        IOManager* process_worker = IOManager::GetThis();

        if (!i.accept_worker.empty()) {
            accept_worker = WorkerMgr::GetInstance().getAsIOManager(i.accept_worker).get();
            if (!accept_worker) {
                FLEXY_LOG_ERROR(g_logger) << "accept_worker: " << i.accept_worker
                << " not exists";
                exit(0);
            }
        }

        if(!i.io_worker.empty()) {
            io_worker = WorkerMgr::GetInstance().getAsIOManager(i.io_worker).get();
            if(!io_worker) {
                FLEXY_LOG_ERROR(g_logger) << "io_worker: " << i.io_worker
                    << " not exists";
                exit(0);
            }
        }

        if(!i.process_worker.empty()) {
            process_worker = WorkerMgr::GetInstance().getAsIOManager(i.process_worker).get();
            if(!process_worker) {
                FLEXY_LOG_ERROR(g_logger) << "process_worker: " << i.process_worker
                    << " not exists";
                exit(0);
            }
        }

        TcpServer::ptr server;

        static std::unordered_map<std::string, std::function<TcpServer::ptr(int, IOManager*, IOManager*, IOManager*)>> tcp_creater = {
            {"http", [](int keep_alive, IOManager* worker, IOManager* io_worker, IOManager* accept_worker) {
                return std::make_shared<http::HttpServer>(keep_alive, worker, io_worker, accept_worker);
            }},
            {"ws", [](int keep_alive, IOManager* worker, IOManager* io_worker, IOManager* accept_worker) {
                return std::make_shared<http::WSServer>(worker, io_worker, accept_worker);
            }},
            {"tcp", [](int keep_alive, IOManager* worker, IOManager* io_worker, IOManager* accept_worker) {
                return std::make_shared<TcpServer>(worker, io_worker, accept_worker);
            }},
        };



        // if (i.type == "http") {
        //     server = std::make_shared<http::HttpServer>(i.keepalive, process_worker, io_worker, accept_worker);
        // } else if (i.type == "ws") {
        //     server = std::make_shared<http::WSServer>(process_worker, io_worker, accept_worker);
        // } else {

        // }
        if (auto it = tcp_creater.find(i.type); it != tcp_creater.end()) {
            server = it->second(i.keepalive, process_worker, io_worker, accept_worker);     // 若不是httpserver, 则忽略keep-alive参数
        } else {
            FLEXY_LOG_ERROR(g_logger) << "invalid server type = " << i.type 
            << LexicalCastYaml<TcpServerConf, std::string>()(i);
            exit(0);
        }

        // auto server = std::make_shared<http::HttpServer>(i.keepalive);
        std::vector<Address::ptr> fails;
        if (!server->bind(address, fails)) {
            for (auto& x : fails) {
                FLEXY_LOG_ERROR(g_logger) << "bind address fail: " << *x;
                exit(0);
            }
        }
        
        if (!i.name.empty()) {
            server->setName(i.name);
        }
        server->setRecvTimeout(i.timeout);
        // server->start();
        servers_[i.type].push_back(std::move(server));
        // httpservers_.push_back(std::move(server));
    }

    if (serverReady) {
        serverReady();
    }

    for (auto& [type, servers] : servers_) {
        for (auto& server : servers) {
            server->start();
        }
    }

    if (serverUp) {
        serverUp();
    }
}

int Application::main(int argc, char** argv) {
    FLEXY_LOG_INFO(g_logger) << "main";
    auto pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();
    std::ofstream ofs(pidfile);
    if (!ofs) {
        FLEXY_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
        return false;
    }
    ofs << getpid();

    IOManager iom(1, true, "main");
    
    go_args(&Application::run_fiber, this);
    iom.addRecTimer(2000, [](){});

    return 0;
}

}