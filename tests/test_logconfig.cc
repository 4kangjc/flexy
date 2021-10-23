#include <flexy/util.h>

static auto&& g_logger = FLEXY_LOG_ROOT();
static auto&& s_logger = FLEXY_LOG_NAME("system");

int main() {
    FLEXY_LOG_INFO(g_logger) << "begin";
    FLEXY_LOG_INFO(g_logger) << "system log";
    YAML::Node node = YAML::LoadFile("../bin/conf/log.yml");
    flexy::Config::LoadFromYaml(node);
    FLEXY_LOG_TRACE(g_logger) << "root log";
    FLEXY_LOG_DEBUG(s_logger) << "system log";
    FLEXY_LOG_INFO(s_logger) << "stdout file server";
    FLEXY_LOG_DEBUG(g_logger) << "root debug";
    sleep(15);
    YAML::Node new_node = YAML::LoadFile("../bin/conf/log.yml");
    flexy::Config::LoadFromYaml(new_node);
    FLEXY_LOG_TRACE(g_logger) << "root trace";
    FLEXY_LOG_INFO(s_logger) << "root level = " << g_logger->getLevel();

    std::ifstream is("../bin/conf/log.json");
    Json::Value v;
    Json::Reader r;
    r.parse(is, v);
    flexy::Config::LoadFromJson(v);

    FLEXY_LOG_INFO(g_logger) << "root logger back";
    FLEXY_LOG_INFO(s_logger) << "system server log";
}