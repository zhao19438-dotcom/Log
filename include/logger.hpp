#ifndef __LOG_LOGGER_HPP__
#define __LOG_LOGGER_HPP__

#include "util.hpp"
#include "level.hpp"
#include "message.hpp"
#include "formatter.hpp"
#include "sink.hpp"
#include "looper.hpp"
#include "config.hpp"

#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <cstdarg>
#include <memory>

namespace logger {

// 跨平台安全格式化字符串辅助函数
inline int safe_vasprintf(char **strp, const char *fmt, va_list ap) {
#ifdef _WIN32
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int len = _vscprintf(fmt, ap_copy);
    va_end(ap_copy);
    if (len < 0) return -1;
    *strp = static_cast<char *>(malloc(len + 1));
    if (!*strp) return -1;
    return vsprintf_s(*strp, len + 1, fmt, ap);
#else
    return vasprintf(strp, fmt, ap);
#endif
}

class SyncLogger;
class AsyncLogger;

// 日志器抽象基类
class Logger {
public:
    enum class Type {
        LOGGER_SYNC = 0,
        LOGGER_ASYNC
    };

    using ptr = std::shared_ptr<Logger>;

    Logger(const std::string &name,
           Formatter::ptr formatter,
           const std::vector<LogSink::ptr> &sinks,
           LogLevel::value level = LogLevel::value::DEBUG)
        : _name(name), _level(level), _formatter(formatter), _sinks(sinks) {}

    virtual ~Logger() = default;

    const std::string &loggerName() const { return _name; }
    LogLevel::value loggerLevel() const { return _level.load(); }

    // 检查日志级别是否满足输出门槛
    LOG_NODISCARD bool shouldLog(LogLevel::value level) const {
        return level >= _level.load();
    }

    // 统一核心提交层：无论上层来自 printf 还是 C++ 流式，最终都经由这里汇合
    void submit(LogLevel::value level, const char *file, size_t line, std::string &&msg) {
        if (!shouldLog(level)) return;
        LogMsg lm(_name, file, line, std::move(msg), level);
        std::stringstream ss;
        _formatter->format(ss, lm);
        logIt(ss.str());
    }

    // 变参接口（C 风格）
    void debug(const char *file, size_t line, const char *fmt, ...) {
        if (!shouldLog(LogLevel::value::DEBUG)) return;
        va_list al;
        va_start(al, fmt);
        log(LogLevel::value::DEBUG, file, line, fmt, al);
        va_end(al);
    }

    void info(const char *file, size_t line, const char *fmt, ...) {
        if (!shouldLog(LogLevel::value::INFO)) return;
        va_list al;
        va_start(al, fmt);
        log(LogLevel::value::INFO, file, line, fmt, al);
        va_end(al);
    }

    void warn(const char *file, size_t line, const char *fmt, ...) {
        if (!shouldLog(LogLevel::value::WARN)) return;
        va_list al;
        va_start(al, fmt);
        log(LogLevel::value::WARN, file, line, fmt, al);
        va_end(al);
    }

    void error(const char *file, size_t line, const char *fmt, ...) {
        if (!shouldLog(LogLevel::value::ERROR)) return;
        va_list al;
        va_start(al, fmt);
        log(LogLevel::value::ERROR, file, line, fmt, al);
        va_end(al);
    }

    void fatal(const char *file, size_t line, const char *fmt, ...) {
        if (!shouldLog(LogLevel::value::FATAL)) return;
        va_list al;
        va_start(al, fmt);
        log(LogLevel::value::FATAL, file, line, fmt, al);
        va_end(al);
    }

#if LOG_HAS_SOURCE_LOCATION
    // C++20 前沿特性：无宏优雅原生调用（自动注入调用者源文件名与代码行号）
    void debug(const std::string &msg, const std::source_location loc = std::source_location::current()) {
        submit(LogLevel::value::DEBUG, loc.file_name(), loc.line(), std::string(msg));
    }
    void info(const std::string &msg, const std::source_location loc = std::source_location::current()) {
        submit(LogLevel::value::INFO, loc.file_name(), loc.line(), std::string(msg));
    }
    void warn(const std::string &msg, const std::source_location loc = std::source_location::current()) {
        submit(LogLevel::value::WARN, loc.file_name(), loc.line(), std::string(msg));
    }
    void error(const std::string &msg, const std::source_location loc = std::source_location::current()) {
        submit(LogLevel::value::ERROR, loc.file_name(), loc.line(), std::string(msg));
    }
    void fatal(const std::string &msg, const std::source_location loc = std::source_location::current()) {
        submit(LogLevel::value::FATAL, loc.file_name(), loc.line(), std::string(msg));
    }
#endif

protected:
    void log(LogLevel::value level, const char *file, size_t line, const char *fmt, va_list al) {
        char *buf = nullptr;
        std::string msg;
        int len = safe_vasprintf(&buf, fmt, al);
        if (len < 0) {
            msg = "[Log Error] 格式化日志消息失败！";
        } else {
            msg.assign(buf, len);
            free(buf);
        }
        submit(level, file, line, std::move(msg));
    }

    // 具体的刷新落地交由派生类（同步或异步）实现
    virtual void logIt(const std::string &msg) = 0;

protected:
    std::mutex _mutex;
    std::string _name;
    std::atomic<LogLevel::value> _level;
    Formatter::ptr _formatter;
    std::vector<LogSink::ptr> _sinks;
};

// 同步日志器
class SyncLogger : public Logger {
public:
    using ptr = std::shared_ptr<SyncLogger>;

    SyncLogger(const std::string &name,
               Formatter::ptr formatter,
               const std::vector<LogSink::ptr> &sinks,
               LogLevel::value level = LogLevel::value::DEBUG)
        : Logger(name, formatter, sinks, level) {}

protected:
    void logIt(const std::string &msg) override {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_sinks.empty()) return;
        for (auto &sink : _sinks) {
            sink->log(msg.data(), msg.size());
        }
    }
};

// 异步日志器
class AsyncLogger : public Logger {
public:
    using ptr = std::shared_ptr<AsyncLogger>;

    AsyncLogger(const std::string &name,
                Formatter::ptr formatter,
                const std::vector<LogSink::ptr> &sinks,
                LogLevel::value level = LogLevel::value::DEBUG)
        : Logger(name, formatter, sinks, level),
          _looper(std::make_shared<AsyncLooper>([this](Buffer &buf) {
              realLog(buf);
          })) {}

protected:
    void logIt(const std::string &msg) override {
        _looper->push(msg.data(), msg.size());
    }

    // 后台专属工作线程批量落盘真实逻辑
    void realLog(Buffer &buf) {
        if (_sinks.empty()) return;
        for (auto &sink : _sinks) {
            sink->log(buf.begin(), buf.readAbleSize());
        }
    }

private:
    AsyncLooper::ptr _looper;
};

// 建造者基类（Builder 模式）
class LoggerBuilder {
public:
    using ptr = std::shared_ptr<LoggerBuilder>;

    LoggerBuilder()
        : _logger_type(Logger::Type::LOGGER_SYNC),
          _level(LogLevel::value::DEBUG) {}

    virtual ~LoggerBuilder() = default;

    void buildLoggerName(const std::string &name) { _logger_name = name; }
    void buildLoggerLevel(LogLevel::value level) { _level = level; }
    void buildLoggerType(Logger::Type type) { _logger_type = type; }

    void buildFormatter(const std::string &pattern) {
        _formatter = std::make_shared<Formatter>(pattern);
    }
    void buildFormatter(const Formatter::ptr &formatter) {
        _formatter = formatter;
    }

    template <typename SinkType, typename ...Args>
    void buildSink(Args &&...args) {
        auto sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
        _sinks.push_back(sink);
    }

    virtual Logger::ptr build() = 0;

protected:
    Logger::Type _logger_type;
    std::string _logger_name;
    LogLevel::value _level;
    Formatter::ptr _formatter;
    std::vector<LogSink::ptr> _sinks;
};

// 本地局部日志器建造者（不注册进单例管理器）
class LocalLoggerBuilder : public LoggerBuilder {
public:
    Logger::ptr build() override {
        assert(!_logger_name.empty());
        if (!_formatter) {
            _formatter = std::make_shared<Formatter>();
        }
        if (_sinks.empty()) {
            buildSink<StdoutSink>();
        }
        if (_logger_type == Logger::Type::LOGGER_ASYNC) {
            return std::make_shared<AsyncLogger>(_logger_name, _formatter, _sinks, _level);
        }
        return std::make_shared<SyncLogger>(_logger_name, _formatter, _sinks, _level);
    }
};

class LoggerManager;

// 全局日志器建造者（自动注册进单例管理器）
class GlobalLoggerBuilder : public LoggerBuilder {
public:
    Logger::ptr build() override;
};

// 日志器单例管理器（Meyers 单例模式）
class LoggerManager {
public:
    static LoggerManager &getInstance() {
        static LoggerManager instance;
        return instance;
    }

    void addLogger(const Logger::ptr &logger) {
        if (hasLogger(logger->loggerName())) return;
        std::unique_lock<std::mutex> lock(_mutex);
        _loggers[logger->loggerName()] = logger;
    }

    bool hasLogger(const std::string &name) {
        std::unique_lock<std::mutex> lock(_mutex);
        return _loggers.find(name) != _loggers.end();
    }

    Logger::ptr getLogger(const std::string &name) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _loggers.find(name);
        if (it == _loggers.end()) {
            return rootLogger();
        }
        return it->second;
    }

    Logger::ptr rootLogger() {
        return _root_logger;
    }

private:
    LoggerManager() {
        // 默认初始化一个根日志器（名为 root，输出到控制台）
        std::unique_ptr<LoggerBuilder> builder(new LocalLoggerBuilder());
        builder->buildLoggerName("root");
        _root_logger = builder->build();
        _loggers["root"] = _root_logger;
    }

    LoggerManager(const LoggerManager &) = delete;
    LoggerManager &operator=(const LoggerManager &) = delete;

private:
    std::mutex _mutex;
    Logger::ptr _root_logger;
    std::unordered_map<std::string, Logger::ptr> _loggers;
};

// 实现 GlobalLoggerBuilder::build
inline Logger::ptr GlobalLoggerBuilder::build() {
    assert(!_logger_name.empty());
    if (!_formatter) {
        _formatter = std::make_shared<Formatter>();
    }
    if (_sinks.empty()) {
        buildSink<StdoutSink>();
    }
    Logger::ptr logger;
    if (_logger_type == Logger::Type::LOGGER_ASYNC) {
        logger = std::make_shared<AsyncLogger>(_logger_name, _formatter, _sinks, _level);
    } else {
        logger = std::make_shared<SyncLogger>(_logger_name, _formatter, _sinks, _level);
    }
    LoggerManager::getInstance().addLogger(logger);
    return logger;
}

} // namespace logger

#endif // __LOG_LOGGER_HPP__
