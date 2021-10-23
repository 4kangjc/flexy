#pragma once

#include "flexy/thread/mutex.h"
#include "singleton.h"
#include "util.h"
#include <sstream>
#include <fmt/format.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <fstream>

namespace flexy {

// 前置申明
class Logger;
class LogAppender;
class LogFormatter;
class Socket;
class IPAddress;


/********************************** 日志级别 *****************************************/
struct LogLevel {
    enum Level {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        UNKOWN
    };
    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(std::string_view s);
};

inline std::ostream& operator<<(std::ostream& os, LogLevel::Level level) {
    return os << LogLevel::ToString(level);
}


/**************************** 日志上下文 *************************************/

class LogContext {
public:
    using ptr = std::unique_ptr<LogContext>;
    LogContext(std::shared_ptr<Logger>& logger, LogLevel::Level level, const char* flle, 
               const char* func, int32_t line, uint32_t elapse, uint32_t threadId, 
               uint32_t fiberId, uint64_t time, std::string_view thread_name) :
        filename_(flle), funcname_(func), linenum_(line), elapse_(elapse), threadId_(threadId),
        fiberId_(fiberId), time_(time), thread_name_(thread_name), logger_(logger),
        level_(level)  {}
    auto getFile() const { return filename_; }
    auto getLine() const { return linenum_; }
    auto getFunc() const { return funcname_; }
    auto getElapse() const { return elapse_; }
    auto getThreadId() const { return threadId_; }
    auto getFiberId() const { return fiberId_; }
    auto getTime() const { return time_; }
    decltype(auto) getLogger() const { return logger_; }
    auto getLevel() const { return level_; }
    auto getCountent() const { return ss_.str(); }
    auto& getSS() { return ss_; }
    const auto& getThreadName() const { return thread_name_; }

    template <typename... Args>
    void format(const char* fmt, Args&&... args) {
        ss_ << fmt::format(fmt, std::forward<Args>(args)...);
    }
private:
    const char* filename_ = nullptr;                // 文件名
    const char* funcname_ = nullptr;                // 函数名
    int32_t linenum_ = 0;                           // 行号
    uint32_t elapse_ = 0;                           // 线程启动到现在的毫秒数
    uint32_t threadId_ = 0;                         // 线程ID
    uint32_t fiberId_ = 0;                          // 协程ID
    uint64_t time_;                                 // 打印日志的时间
    std::string thread_name_;                       // 线程名称
    std::stringstream ss_;                          // 日志内容流
    std::shared_ptr<Logger>& logger_;               // 日志器 引用形式
    LogLevel::Level level_;                         // 日志等级
};


/********************** 日志格式器 **************************************/
class LogFormatter {
public:
    using ptr = std::shared_ptr<LogFormatter>;
    LogFormatter(std::string_view pattern);
    std::string format(std::shared_ptr<Logger>& logger, LogContext::ptr& context);
    std::ostream& format(std::ostream& os, std::shared_ptr<Logger>& logger, 
                        LogContext::ptr& context);
    bool isError() const { return error_; }
    const auto& getPattern() const { return pattern_; }

    struct FormatItem {
    public:
        using ptr = std::shared_ptr<FormatItem>;
        virtual ~FormatItem() = default;
        virtual void format(std::ostream& os, std::shared_ptr<Logger>& logger, 
                            LogContext::ptr& context) = 0;
    };
private:
    void init();
private:
    std::vector<FormatItem::ptr> items_;        // FormatItem集合
    std::string pattern_;                       // 日志格式
    bool error_ = false;                        // 日志格式有无错误
};

/****************************** 日志器  ***********************************************/

class Logger : public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
    using ptr = std::shared_ptr<Logger>;
    void log(std::unique_ptr<LogContext>& contex);

    void addAppender(const std::shared_ptr<LogAppender>&);
    void delAppender(const std::shared_ptr<LogAppender>&);
    void clearAppender() { LOCK_GUARD(mutex_);  appenders_.clear(); }
    auto getLevel() const { return level_; }
    void setLevel(LogLevel::Level level) { level_ = level; }
    void setFormatter(const std::shared_ptr<LogFormatter>& val);
    void setFormatter(std::string_view fmt);
    auto getFormatter() const;
    const auto& getName() const { return name_; }
    std::string toYamlString() const;
    std::string toJsonString() const;
private:
    Logger(std::string_view name = "root");         // 只能由LoggerManager调用
private:
    std::string name_;                                                      // 日志名称
    LogLevel::Level level_ = LogLevel::DEBUG;                               // 日志等级
    std::unordered_set<std::shared_ptr<LogAppender>> appenders_;            // 日志输出地集合
    LogFormatter::ptr formatter_;                                           // 日志格式
    Logger::ptr root_;                                                      // 主日志器
    mutable Spinlock mutex_;                                                // 自旋锁
};


/**************************** 日志输出地 ******************************************/

class LogAppender {
friend class Logger;
public:
    using ptr = std::shared_ptr<LogAppender>;
    virtual ~LogAppender() = default;
    virtual void log(Logger::ptr& logger, LogContext::ptr& contex) = 0;
    virtual std::string toYamlString() const = 0;
    virtual std::string toJsonString() const = 0;
    void setFormatter(const LogFormatter::ptr& val);
    auto& getFormatter() const { LOCK_GUARD(mutex_); return formatter_; }
    auto getLevel() const { return level_; }
    void setLevel(LogLevel::Level level) { level_ = level; }
protected:
    LogLevel::Level level_ = LogLevel::DEBUG;       // 日志级别
    LogFormatter::ptr formatter_;                   // 日志格式
    bool hasFormatter_ = false;                     // 是否有自己的日志格式
    mutable Spinlock mutex_;                        // 自旋锁
};

// 输出到终端的Appender
class StdoutLogAppender : public LogAppender {
public:
    void log(Logger::ptr& logger, LogContext::ptr& contex) override;
    std::string toYamlString() const override;
    std::string toJsonString() const override;
};

// 输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
    FileLogAppender(std::string_view filename);
    void log(Logger::ptr& logger, LogContext::ptr& contex) override;
    std::string toYamlString() const override;
    std::string toJsonString() const override;
    //重新打开文件，文件打开成功返回true
    bool reopen();
private:
    std::string filename_;
    std::ofstream filestream_;
};

// 输出到服务器的Appender
class ServerLogAppender : public LogAppender {
public:
    ServerLogAppender(std::string_view host);
    void log(Logger::ptr& logger, LogContext::ptr& contex) override;
    std::string toYamlString() const override;
    std::string toJsonString() const override;
private:
    std::shared_ptr<Socket> sock_;
    std::shared_ptr<IPAddress> addr_;
};

/****************************** 日志上下文包装器 *****************************/

class LogContextWrap {
public:
    using ptr = std::unique_ptr<LogContext>;
    LogContextWrap(std::unique_ptr<LogContext>&& e) : contex_(std::move(e)) { }
    ~LogContextWrap() { contex_->getLogger()->log(contex_); }
    auto& getSS() { return contex_->getSS(); }
    auto& getContex() { return contex_; }
private:
    ptr contex_;
};

/*******************************  日志器管理类 ********************************/

class LoggerManager {
friend class Singleton<LoggerManager>;
public:
    Logger::ptr getLogger(const std::string& name);
    auto& getRoot() { return root_; }
    std::string toYamlString();
private:
    LoggerManager();                                                // 只能由单例类调用 
    std::unordered_map<std::string, Logger::ptr> loggers_;          // 日志器集合
    Logger::ptr root_;                                              // 主日志器
    mutable Spinlock mutex_;                                        // 自旋锁
};

using LoggerMgr = Singleton<LoggerManager>;


/***************************  日志宏  **************************************/

#define FLEXY_LOG_LEVEL(logger, level) \
    if (logger->getLevel() <= level) \
        flexy::LogContextWrap(std::make_unique<flexy::LogContext>(logger, level, \
        __FILE__, __func__, __LINE__, flexy::GetThreadElapse(), flexy::GetThreadId(), \
        flexy::GetFiberId(), flexy::GetTimeMs() / 1000, flexy::GetThreadName())).getSS()

#define FLEXY_LOG_TRACE(logger)   FLEXY_LOG_LEVEL(logger, flexy::LogLevel::TRACE)
#define FLEXY_LOG_DEBUG(logger)   FLEXY_LOG_LEVEL(logger, flexy::LogLevel::DEBUG)
#define FLEXY_LOG_INFO(logger)    FLEXY_LOG_LEVEL(logger, flexy::LogLevel::INFO )
#define FLEXY_LOG_WARN(logger)    FLEXY_LOG_LEVEL(logger, flexy::LogLevel::WARN )
#define FLEXY_LOG_ERROR(logger)   FLEXY_LOG_LEVEL(logger, flexy::LogLevel::ERROR)
#define FLEXY_LOG_FATAL(logger)   FLEXY_LOG_LEVEL(logger, flexy::LogLevel::FATAL)

#define FLEXY_LOG_FMT_LEVEL(logger, level, fmt, args...) \
    if (logger->getLevel() <= level) \
        flexy::LogContextWrap(std::make_unique<flexy::LogContext>(logger, level, \
        __FILE__, __func__, __LINE__, flexy::GetThreadElapse(), flexy::GetThreadId(), \
        flexy::GetFiberId(), flexy::GetTimeMs() / 1000, flexy::GetThreadName())).getContex()->format(fmt, ##args)

#define FLEXY_LOG_FMT_TRACE(logger, fmt, args...)   FLEXY_LOG_FMT_LEVEL(logger, flexy::LogLevel::TRACE, fmt, ##args)
#define FLEXY_LOG_FMT_DEBUG(logger, fmt, args...)   FLEXY_LOG_FMT_LEVEL(logger, flexy::LogLevel::DEBUG, fmt, ##args)
#define FLEXY_LOG_FMT_INFO(logger,  fmt, args...)   FLEXY_LOG_FMT_LEVEL(logger, flexy::LogLevel::INFO,  fmt, ##args)
#define FLEXY_LOG_FMT_WARN(logger,  fmt, args...)   FLEXY_LOG_FMT_LEVEL(logger, flexy::LogLevel::WARN,  fmt, ##args)
#define FLEXY_LOG_FMT_ERROR(logger, fmt, args...)   FLEXY_LOG_FMT_LEVEL(logger, flexy::LogLevel::ERROR, fmt, ##args)
#define FLEXY_LOG_FMT_FATAL(logger, fmt, args...)   FLEXY_LOG_FMT_LEVEL(logger, flexy::LogLevel::FATAL, fmt, ##args)

#define FLEXY_LOG_ROOT()        flexy::LoggerMgr::GetInstance().getRoot()
#define FLEXY_LOG_NAME(name)    flexy::LoggerMgr::GetInstance().getLogger(name)

} // namespace flexy