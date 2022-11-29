#include <flexy/util/log.h>

static auto&& g_logger = FLEXY_LOG_ROOT();

int main() {
    FLEXY_LOG_INFO(g_logger) << "Hello flexy log!";
    FLEXY_LOG_FMT_INFO(g_logger, "hello {}", "format");
    FLEXY_LOG_TRACE(g_logger) << "trace";
    flexy::LogAppender::ptr file(new flexy::FileLogAppender("../bin/conf/log.txt"));   
    g_logger->addAppender(file);
    FLEXY_LOG_INFO(g_logger) << "log.txt";
    auto s_logger = FLEXY_LOG_NAME("test");
    FLEXY_LOG_DEBUG(s_logger) << "test s_lgger";
    s_logger->setLevel(flexy::LogLevel::ERROR);
    flexy::LogAppender::ptr StdOut(new flexy::StdoutLogAppender);
    s_logger->addAppender(StdOut);
    FLEXY_LOG_FMT_WARN(s_logger, "add appender");

    // auto server = std::make_shared<flexy::ServerLogAppender>("121.41.169.58:2099");
    // g_logger->addAppender(server);
    // FLEXY_LOG_FMT_INFO(g_logger, "hello flexy server log");
}