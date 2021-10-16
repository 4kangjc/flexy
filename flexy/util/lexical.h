#pragma once 

#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <json/json.h>

namespace flexy {

/************************* Yaml  ************************************/

template <typename F, typename T>
struct LexicalCastYaml {
    T operator() (const F& v) {
        return boost::lexical_cast<T>(v);
    }
};

// vector
template <typename T>
struct LexicalCastYaml<std::string, std::vector<T>> {
    decltype(auto) operator() (const std::string& v) {
        auto node = YAML::Load(v);
        std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.emplace_back(LexicalCastYaml<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastYaml<std::vector<T>, std::string> {
    decltype(auto) operator() (const std::vector<T>& vec) {
        YAML::Node node;
        for (auto& v : vec) {
            node.push_back(YAML::Load(LexicalCastYaml<T, std::string>()(v)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// list
template <typename T>
struct LexicalCastYaml<std::string, std::list<T>> {
    decltype(auto) operator() (const std::string v) {
        auto node = YAML::Load(v);
        std::list<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.emplace_back(LexicalCastYaml<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastYaml<std::list<T>, std::string> {
    decltype(auto) operator() (const std::list<T>& vec) {
        YAML::Node node;
        for (auto& v : vec) {
            node.push_back(YAML::Load(LexicalCastYaml<T, std::string>()(v)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// set
template <typename T>
struct LexicalCastYaml<std::string, std::set<T>> {
    decltype(auto) operator() (const std::string v) {
        auto node = YAML::Load(v);
        std::set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.emplace(LexicalCastYaml<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastYaml<std::set<T>, std::string> {
    decltype(auto) operator() (const std::set<T>& vec) {
        YAML::Node node;
        for (auto& v : vec) {
            node.push_back(YAML::Load(LexicalCastYaml<T, std::string>()(v)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// unordered_set
template <typename T>
struct LexicalCastYaml<std::string, std::unordered_set<T>> {
    decltype(auto) operator() (const std::string v) {
        auto node = YAML::Load(v);
        std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.emplace(LexicalCastYaml<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastYaml<std::unordered_set<T>, std::string> {
    decltype(auto) operator() (const std::unordered_set<T>& vec) {
        YAML::Node node;
        for (auto& v : vec) {
            node.push_back(YAML::Load(LexicalCastYaml<T, std::string>()(v)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// map
template <typename T>
struct LexicalCastYaml<std::string, std::map<std::string, T>> {
    decltype(auto) operator() (const std::string v) {
        auto node = YAML::Load(v);
        std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.emplace(it->first.Scalar(), LexicalCastYaml<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastYaml<std::map<std::string, T>, std::string> {
    decltype(auto) operator() (const std::map<std::string, T>& vec) {
        YAML::Node node;
        for (const auto& [x, y] : vec) {
            node[x] = YAML::Load(LexicalCastYaml<T, std::string>()(y));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// unordered_map
template <typename T>
struct LexicalCastYaml<std::string, std::unordered_map<std::string, T>> {
    decltype(auto) operator() (const std::string v) {
        auto node = YAML::Load(v);
        std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.emplace(it->first.Scalar(), LexicalCastYaml<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastYaml<std::unordered_map<std::string, T>, std::string> {
    decltype(auto) operator() (const std::unordered_map<std::string, T>& vec) {
        YAML::Node node;
        for (const auto& [x, y] : vec) {
            node[x] = YAML::Load(LexicalCastYaml<T, std::string>()(y));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/******************************  Json  *************************************/

template <typename F, typename T>
struct LexicalCastJson {
    T operator() (const F& v) {
        return boost::lexical_cast<T>(v);
    }
};

// vector
template <typename T>
struct LexicalCastJson<std::string, std::vector<T>> {
    decltype(auto) operator() (const std::string& v) {
        Json::Reader reader;
        Json::Value node;
        reader.parse(v, node);
        std::vector<T> vec;
        int n = node.size();
        std::stringstream ss;
        for (int i = 0; i < n; ++i) {
            ss.str("");
            ss << node[i];
            vec.emplace_back(LexicalCastJson<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastJson<std::vector<T>, std::string> {
    decltype(auto) operator() (const std::vector<T>& vec) {
        Json::Value node;
        Json::Reader r;
        for (auto& v : vec) {
            Json::Value n;
            r.parse(LexicalCastJson<T, std::string>()(v), n);
            node.append(n);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
        // Json::FastWriter w;
        // return w.write(node);
    }
};

// list
template <typename T>
struct LexicalCastJson<std::string, std::list<T>> {
    decltype(auto) operator() (const std::string& v) {
        Json::Reader reader;
        Json::Value node;
        reader.parse(v, node);
        std::list<T> vec;
        int n = node.size();
        std::stringstream ss;
        for (int i = 0; i < n; ++i) {
            ss.str("");
            ss << node[i];
            vec.emplace_back(LexicalCastJson<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastJson<std::list<T>, std::string> {
    decltype(auto) operator() (const std::list<T>& vec) {
        Json::Value node;
        Json::Reader r;
        for (auto& v : vec) {
            Json::Value n;
            r.parse(LexicalCastJson<T, std::string>()(v), n);
            node.append(n);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
        // Json::FastWriter w;
        // return w.write(node);
    }
};

// set 
template <typename T>
struct LexicalCastJson<std::string, std::set<T>> {
    decltype(auto) operator() (const std::string& v) {
        Json::Reader reader;
        Json::Value node;
        reader.parse(v, node);
        std::set<T> vec;
        int n = node.size();
        std::stringstream ss;
        for (int i = 0; i < n; ++i) {
            ss.str("");
            ss << node[i];
            vec.emplace(LexicalCastJson<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastJson<std::set<T>, std::string> {
    decltype(auto) operator() (const std::set<T>& vec) {
        Json::Value node;
        Json::Reader r;
        for (auto& v : vec) {
            Json::Value n;
            r.parse(LexicalCastJson<T, std::string>()(v), n);
            node.append(n);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
        // Json::FastWriter w;
        // return w.write(node);
    }
};

// unordered_set 
template <typename T>
struct LexicalCastJson<std::string, std::unordered_set<T>> {
    decltype(auto) operator() (const std::string& v) {
        Json::Reader reader;
        Json::Value node;
        reader.parse(v, node);
        std::unordered_set<T> vec;
        int n = node.size();
        std::stringstream ss;
        for (int i = 0; i < n; ++i) {
            ss.str("");
            ss << node[i];
            vec.emplace(LexicalCastJson<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastJson<std::unordered_set<T>, std::string> {
    decltype(auto) operator() (const std::unordered_set<T>& vec) {
        Json::Value node;
        Json::Reader r;
        for (auto& v : vec) {
            Json::Value n;
            r.parse(LexicalCastJson<T, std::string>()(v), n);
            node.append(n);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
        // Json::FastWriter w;
        // return w.write(node);
    }
};

// map
template <typename T>
struct LexicalCastJson<std::string, std::map<std::string, T>> {
    decltype(auto) operator() (const std::string& v) {
        Json::Reader reader;
        Json::Value node;
        reader.parse(v, node);
        std::map<std::string, T> vec;
        auto me = node.getMemberNames();
        std::stringstream ss;
        for (auto it = me.begin(); it != me.end(); ++it) {
            ss.str("");
            ss << node[*it];
            vec.emplace(*it, LexicalCastJson<std::string, T>()((ss.str())));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastJson<std::map<std::string, T>, std::string> {
    decltype(auto) operator() (const std::map<std::string, T>& vec) {
        Json::Value node;
        Json::Reader r;
        for (const auto& [x, y] : vec) {
            Json::Value n;
            r.parse(LexicalCastJson<T, std::string>()(y), n);
            node[x] = n;
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
        // Json::FastWriter w;
        // return w.write(node);
    }
};

// unordered_map
template <typename T>
struct LexicalCastJson<std::string, std::unordered_map<std::string, T>> {
    decltype(auto) operator() (const std::string& v) {
        Json::Reader reader;
        Json::Value node;
        reader.parse(v, node);
        std::unordered_map<std::string, T> vec;
        auto me = node.getMemberNames();
        std::stringstream ss;
        for (auto it = me.begin(); it != me.end(); ++it) {
            ss.str("");
            ss << node[*it];
            vec.emplace(*it, LexicalCastJson<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <typename T>
struct LexicalCastJson<std::unordered_map<std::string, T>, std::string> {
    decltype(auto) operator() (const std::unordered_map<std::string, T>& vec) {
        Json::Value node;
        Json::Reader r;
        for (const auto& [x, y] : vec) {
            Json::Value n;
            r.parse(LexicalCastJson<T, std::string>()(y), n);
            node[x] = n;
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
        // Json::FastWriter w;
        // return w.write(node);
    }
};

}

template <typename T>
using YamlFromStr = flexy::LexicalCastYaml<std::string, T>;

template <typename T>
using YamlToStr = flexy::LexicalCastYaml<T, std::string>;

template <typename T>
using JsonFromStr = flexy::LexicalCastJson<std::string, T>;

template <typename T>
using JsonToStr = flexy::LexicalCastJson<T, std::string>;