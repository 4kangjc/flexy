#include <flexy/util/config.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

struct Person {
    std::string name;
    int age = 0;
    bool sex = 0;
    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name = " << name << ", age = " << age << ", sex = " << sex << "]";
        return ss.str();
    }
    bool operator==(const Person& other) const {
        return name == other.name && age == other.age && sex == other.sex;
    }
};

namespace flexy {

template<>
struct LexicalCastYaml<std::string, Person> {
    decltype(auto) operator() (const std::string& v) {
        auto node = YAML::Load(v);
        Person p;
        p.name = node["name"].as<std::string>();
        p.age = node["age"].as<int>();
        p.sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
struct LexicalCastYaml<Person, std::string> {
    decltype(auto) operator() (const Person& p) {
        YAML::Node node;
        node["name"] = p.name;
        node["age"] = p.age;
        node["sex"] = p.sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<>
struct LexicalCastJson<std::string, Person> {
    decltype(auto) operator() (const std::string& v) {
        Json::Reader r;
        Json::Value node;
        r.parse(v, node);
        Person p;
        // p.name = node["name"].as<std::string>();
        // p.age = node["age"].as<int>();
        // p.sex = node["sex"].as<bool>();
        p.name = node["name"].asString();
        p.age = node["age"].asInt();
        p.sex = node["sex"].asBool();
        return p;
    }
};

template<>
struct LexicalCastJson<Person, std::string> {
    decltype(auto) operator() (const Person& p) {
        Json::Value node;
        node["name"] = p.name;
        node["age"] = p.age;
        node["sex"] = p.sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
        // Json::FastWriter w;
        // return w.write(node);
    }
};


}


static auto g_int_value_config = flexy::Config::Lookup<int, JsonFromStr<int>, JsonToStr<int>>
                                                ("system.port", 8080, "system.port");
auto g_vec_value_config = flexy::Config::Lookup("system.int_vec", std::vector{1, 2, 3, 4, 5}, "vector int");
auto g_vec_value_config_json = flexy::Config::Lookup<std::vector<int>, JsonFromStr<std::vector<int>>, JsonToStr<std::vector<int>>>
            ("system.int_vecj", std::vector{1, 2, 3, 4, 5}, "vector int");

void test_config() {
    FLEXY_LOG_INFO(g_logger) << g_int_value_config->toString();
#define XX(g_var, name, prefix)  \
    for (auto& i : g_var->getValue()) \
        FLEXY_LOG_INFO(g_logger) << #prefix << " " << #name << ": " << i;   

    XX(g_vec_value_config, int_vec, before);
    auto vec_str = g_vec_value_config->toString();
    FLEXY_LOG_INFO(g_logger) << vec_str;
    for (auto i : YamlFromStr<std::vector<int>>()(vec_str)) {
        FLEXY_LOG_INFO(g_logger) << i;
    }
    auto vec_str_json = g_vec_value_config_json->toString();
    FLEXY_LOG_INFO(g_logger) << vec_str_json;
    for (auto i : JsonFromStr<std::vector<int>>()(vec_str_json)) {
        FLEXY_LOG_INFO(g_logger) << i;
    }

}

auto person_value_config = flexy::Config::Lookup("user.person", Person{"yyds", 1, false}, "user person");
auto person_value_configj = flexy::Config::Lookup<Person, JsonFromStr<Person>, JsonToStr<Person>>("user.personj", Person{"yyds", 1, false}, "user person");


void test_person() {
    FLEXY_LOG_INFO(g_logger) << person_value_config->toString();
    std::vector<Person> vecp;
    for (int i = 0; i < 6; ++i) {
        Person p{"p" + std::to_string(i), i, false};
        vecp.push_back(p);
    }
    auto pervec_value_config = flexy::Config::Lookup("user.pervec", vecp, "person vector");
    auto pervec_str = pervec_value_config->toString();
    FLEXY_LOG_INFO(g_logger) << pervec_str;
    for (auto& i : YamlFromStr<std::vector<Person>>()(pervec_str)) {
        FLEXY_LOG_INFO(g_logger) << i.toString();
    }

    auto pervec_value_configj = flexy::Config::Lookup<std::vector<Person>, JsonFromStr<std::vector<Person>>, JsonToStr<std::vector<Person>>>("user.pervecj", vecp, "person vector json");
    auto pervec_strj = pervec_value_configj->toString();
    FLEXY_LOG_INFO(g_logger) << pervec_strj;
        for (auto& i : JsonFromStr<std::vector<Person>>()(pervec_strj)) {
        FLEXY_LOG_INFO(g_logger) << i.toString();
    }
    auto mappervec = std::map<std::string, std::vector<Person>>();
    mappervec["hello"] = vecp;
    mappervec["world"] = vecp;
    auto person_vec_map = flexy::Config::Lookup("class.map_vec", mappervec, "person map vector");
    FLEXY_LOG_INFO(g_logger) << person_vec_map->toString();

    std::map<std::string, Person> mp;
    mp["hello"] = vecp[0];
    mp["cpp"] = vecp[2];
    mp["flexy"] = vecp[4];
    auto map_int_j = flexy::Config::Lookup<decltype(mp), JsonFromStr<decltype(mp)>, JsonToStr<decltype(mp)>>("map.int", mp, "map string int");
    FLEXY_LOG_INFO(g_logger) << map_int_j->toString();

    using map_vec_person = decltype(mappervec);
    auto person_vec_mapj = flexy::Config::Lookup<map_vec_person, JsonFromStr<map_vec_person>, JsonToStr<map_vec_person>>("class.map_vecj", mappervec, "person map vector");
    FLEXY_LOG_INFO(g_logger) << person_vec_mapj->toString();

    auto temp = JsonFromStr<map_vec_person>()(person_vec_mapj->toString());
    if (temp != mappervec) {
        FLEXY_LOG_ERROR(g_logger) << "Errot Json From Str";
    } else {
        FLEXY_LOG_INFO(g_logger) << "Success";
    }
}


void test_loadfile() {
    YAML::Node root = YAML::LoadFile("../bin/conf/test.yml");
    // FLEXY_LOG_INFO(g_logger) << root;
    flexy::Config::LoadFromYaml(root);
    FLEXY_LOG_INFO(g_logger) << g_vec_value_config->toString();
    FLEXY_LOG_INFO(g_logger) << person_value_config->toString();

    std::ifstream is("../bin/conf/test.json");
    Json::Reader r;
    Json::Value val;
    r.parse(is, val);
    flexy::Config::LoadFromJson(val);
    FLEXY_LOG_INFO(g_logger) << g_vec_value_config_json->toString();
    FLEXY_LOG_INFO(g_logger) << person_value_configj->toString();
    FLEXY_LOG_INFO(g_logger) << g_int_value_config->getValue();

    // FLEXY_LOG_INFO(g_logger) << val;
}


int main() {
    // test_config();
    // test_person();
    test_loadfile();
}