#pragma once
#ifndef LOGGER_LOGGER_HPP_
#define LOGGER_LOGGER_HPP_

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

// 格式化变参字符串
inline std::string format_vstring(const char *fmt, va_list ap) {
    va_list ap_copy;
    va_copy(ap_copy, ap);
#ifdef _WIN32
    int len = _vscprintf(fmt, ap_copy);
#else
    int len = vsnprintf(nullptr, 0, fmt, ap_copy);
#endif
    va_end(ap_copy);
    if (len < 0) {
        return "[Log Error] 格式化日志消息失败！";
    }
    std::string result(len, '\0');
#ifdef _WIN32
    vsprintf_s(&result[0], len + 1, fmt, ap);
#else
    vsnprintf(&result[0], len + 1, fmt, ap);
#endif
    return result;
}

class SyncLogger;
class AsyncLogger;

// 日志器基类
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

    // 获取日志器名称
    const std::string &loggerName() const { return _name; }
    // 获取当前日志输出门槛级别
    LogLevel::value loggerLevel() const { return _level.load(); }
    // 动态调整日志输出门槛级别
    void setLevel(LogLevel::value level) { _level.store(level); }

    // 判断指定级别是否达到当前日志器的输出门槛
    LOG_NODISCARD bool shouldLog(LogLevel::value level) const {
        return level >= _level.load();
    }

    // 格式化并提交日志消息
    // @param level 日志等级
    // @param file 源文件名
    // @param line 代码行号
    // @param msg 日志正文内容
    void submit(LogLevel::value level, const char *file, size_t line, std::string &&msg) {
        if (!shouldLog(level)) return;
        LogMsg lm(_name, file, line, std::move(msg), level);
        std::string out;
        out.reserve(256);
        _formatter->format(out, lm);
        logIt(out);
    }

#define LOG_VARIADIC_IMPL(level_val) \
    if (!shouldLog(level_val)) return; \
    va_list al; \
    va_start(al, fmt); \
    log(level_val, file, line, fmt, al); \
    va_end(al)

    // C 风格变参日志输出接口
    void debug(const char *file, size_t line, const char *fmt, ...) {
        LOG_VARIADIC_IMPL(LogLevel::value::DEBUG);
    }

    void info(const char *file, size_t line, const char *fmt, ...) {
        LOG_VARIADIC_IMPL(LogLevel::value::INFO);
    }

    void warn(const char *file, size_t line, const char *fmt, ...) {
        LOG_VARIADIC_IMPL(LogLevel::value::WARN);
    }

    void error(const char *file, size_t line, const char *fmt, ...) {
        LOG_VARIADIC_IMPL(LogLevel::value::ERROR);
    }

    void fatal(const char *file, size_t line, const char *fmt, ...) {
        if (shouldLog(LogLevel::value::FATAL)) {
            va_list al;
            va_start(al, fmt);
            log(LogLevel::value::FATAL, file, line, fmt, al);
            va_end(al);
        }
        std::abort();
    }

#undef LOG_VARIADIC_IMPL

#if LOG_HAS_SOURCE_LOCATION
    // source_location 调用接口（自动获取文件名与行号）
    void debug(const std::string &msg, const std::source_location &loc = std::source_location::current()) {
        submit(LogLevel::value::DEBUG, loc.file_name(), loc.line(), std::string(msg));
    }
    void info(const std::string &msg, const std::source_location &loc = std::source_location::current()) {
        submit(LogLevel::value::INFO, loc.file_name(), loc.line(), std::string(msg));
    }
    void warn(const std::string &msg, const std::source_location &loc = std::source_location::current()) {
        submit(LogLevel::value::WARN, loc.file_name(), loc.line(), std::string(msg));
    }
    void error(const std::string &msg, const std::source_location &loc = std::source_location::current()) {
        submit(LogLevel::value::ERROR, loc.file_name(), loc.line(), std::string(msg));
    }
    void fatal(const std::string &msg, const std::source_location &loc = std::source_location::current()) {
        submit(LogLevel::value::FATAL, loc.file_name(), loc.line(), std::string(msg));
        std::abort();
    }
#endif

protected:
    void log(LogLevel::value level, const char *file, size_t line, const char *fmt, va_list al) {
        submit(level, file, line, format_vstring(fmt, al));
    }

public:
    virtual void flush() = 0;

protected:
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

public:
    void flush() override {
        std::unique_lock<std::mutex> lock(_mutex);
        for (auto &sink : _sinks) {
            sink->flush();
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
    ~AsyncLogger() { _looper->stop(); }

protected:
    void logIt(const std::string &msg) override {
        _looper->push(msg.data(), msg.size());
    }

    // 将缓冲区日志写入各落地端
    void realLog(Buffer &buf) {
        if (_sinks.empty()) return;
        for (auto &sink : _sinks) {
            sink->log(buf.begin(), buf.readAbleSize());
        }
    }

public:
    void flush() override {
        _looper->flush();
        for (auto &sink : _sinks) {
            sink->flush();
        }
    }

private:
    AsyncLooper::ptr _looper;
};

// 日志器建造者基类
class LoggerBuilder {
public:
    using ptr = std::shared_ptr<LoggerBuilder>;

    LoggerBuilder()
        : _logger_type(Logger::Type::LOGGER_SYNC),
          _level(LogLevel::value::DEBUG) {}

    virtual ~LoggerBuilder() = default;

    // 设置日志器名称
    void buildLoggerName(const std::string &name) { _logger_name = name; }
    // 设置过滤级别
    void buildLoggerLevel(LogLevel::value level) { _level = level; }
    // 设置日志器类型（同步/异步）
    void buildLoggerType(Logger::Type type) { _logger_type = type; }

    // 设置格式化模式
    void buildFormatter(const std::string &pattern) {
        _formatter = std::make_shared<Formatter>(pattern);
    }
    void buildFormatter(const Formatter::ptr &formatter) {
        _formatter = formatter;
    }

    // 添加落地端
    template <typename SinkType, typename ...Args>
    void buildSink(Args &&...args) {
        auto sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
        _sinks.push_back(sink);
    }

    // 构建日志器实例
    virtual Logger::ptr build() {
        if (_logger_name.empty()) {
            throw std::invalid_argument("[Log Error] Logger name cannot be empty");
        }
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

protected:
    Logger::Type _logger_type;
    std::string _logger_name;
    LogLevel::value _level;
    Formatter::ptr _formatter;
    std::vector<LogSink::ptr> _sinks;
};

// 本地日志器建造者（构建后不自动注册到全局管理器）
class LocalLoggerBuilder : public LoggerBuilder {
};

class LoggerManager;

// 全局日志器建造者（构建后自动注册到全局单例管理器）
class GlobalLoggerBuilder : public LoggerBuilder {
public:
    Logger::ptr build() override;
};

// 全局日志器单例管理器
class LoggerManager {
public:
    static LoggerManager &getInstance() {
        static LoggerManager instance;
        return instance;
    }

    // 添加日志器
    void addLogger(const Logger::ptr &logger) {
        if (!logger) return;
        std::unique_lock<std::mutex> lock(_mutex);
        const auto &name = logger->loggerName();
        if (_loggers.find(name) != _loggers.end()) {
            throw std::invalid_argument("Logger already exists: " + name);
        }
        _loggers[name] = logger;
    }

    // 检查是否存在指定名称的日志器
    bool hasLogger(const std::string &name) {
        std::unique_lock<std::mutex> lock(_mutex);
        return _loggers.find(name) != _loggers.end();
    }

    // 获取指定名称的日志器
    Logger::ptr getLogger(const std::string &name) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _loggers.find(name);
        if (it == _loggers.end()) {
            throw std::invalid_argument("Logger not found: " + name);
        }
        return it->second;
    }

    // 获取默认根日志器
    Logger::ptr rootLogger() {
        std::unique_lock<std::mutex> lock(_mutex);
        return _root_logger;
    }

    // 设置默认根日志器
    void setRootLogger(const Logger::ptr &logger) {
        if (!logger) return;
        std::unique_lock<std::mutex> lock(_mutex);
        _root_logger = logger;
        _loggers["root"] = logger;
    }

    // 刷新并排空所有日志器缓冲区
    void shutdown() {
        std::vector<Logger::ptr> loggers_to_flush;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            for (auto &pair : _loggers) {
                if (pair.second) {
                    loggers_to_flush.push_back(pair.second);
                }
            }
        }
        for (auto &logger : loggers_to_flush) {
            logger->flush();
        }
    }

private:
    LoggerManager() {
        auto builder = std::make_unique<LocalLoggerBuilder>();
        builder->buildLoggerName("root");
        _root_logger = builder->build();
        _loggers["root"] = _root_logger;
    }

    ~LoggerManager() {
        shutdown();
    }

    LoggerManager(const LoggerManager &) = delete;
    LoggerManager &operator=(const LoggerManager &) = delete;

private:
    std::mutex _mutex;
    Logger::ptr _root_logger;
    std::unordered_map<std::string, Logger::ptr> _loggers;
};

inline Logger::ptr GlobalLoggerBuilder::build() {
    Logger::ptr logger = LoggerBuilder::build();
    LoggerManager::getInstance().addLogger(logger);
    return logger;
}

} // namespace logger

#endif // LOGGER_LOGGER_HPP_
