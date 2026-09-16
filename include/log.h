#pragma once
#ifndef LOGGER_LOG_H_
#define LOGGER_LOG_H_

#include "config.hpp"
#include "logger.hpp"
#include "stream.hpp"

namespace logger {

// 获取指定名称的日志器（必须声明为 inline，防止多源文件包含时链接报错）
inline Logger::ptr getLogger(const std::string &name) {
    return LoggerManager::getInstance().getLogger(name);
}

// 获取默认根日志器（root）
inline Logger::ptr rootLogger() {
    return LoggerManager::getInstance().rootLogger();
}

namespace detail {
// 抽取公共建造流程，消除门面初始化函数的代码冗余（DRY 原则）
inline Logger::ptr build_helper(LoggerBuilder *builder,
                                const std::string &name,
                                Logger::Type type,
                                LogLevel::value level,
                                const std::string &logfile,
                                size_t roll_size,
                                bool console,
                                const std::string &pattern) {
    builder->buildLoggerName(name);
    builder->buildLoggerLevel(level);
    builder->buildLoggerType(type);
    if (!pattern.empty()) {
        builder->buildFormatter(pattern);
    }
    if (console) {
        builder->buildSink<StdoutSink>();
    }
    if (!logfile.empty()) {
        if (roll_size > 0) {
            builder->buildSink<RollSink>(logfile, roll_size);
        } else {
            builder->buildSink<FileSink>(logfile);
        }
    }
    return builder->build();
}
} // namespace detail

// ==========================================
// 快捷初始化函数族（极简门面，1 行搞定初始化）
// 生产级防爆：默认日志过滤等级设为 INFO
// ==========================================

// 1. 纯控制台日志（开箱即用，零配置）
inline void init_console(LogLevel::value level = LogLevel::value::INFO,
                         const std::string &pattern = "") {
    std::unique_ptr<LoggerBuilder> builder(new LocalLoggerBuilder());
    LoggerManager::getInstance().setRootLogger(
        detail::build_helper(builder.get(), "root", Logger::Type::LOGGER_SYNC, level, "", 0, true, pattern));
}

// 2. 同步文件日志（可选择是否同步输出到控制台）
inline void init_sync(const std::string &logfile,
                      LogLevel::value level = LogLevel::value::INFO,
                      bool console = true,
                      const std::string &pattern = "") {
    std::unique_ptr<LoggerBuilder> builder(new LocalLoggerBuilder());
    LoggerManager::getInstance().setRootLogger(
        detail::build_helper(builder.get(), "root", Logger::Type::LOGGER_SYNC, level, logfile, 0, console, pattern));
}

// 3. 高性能异步日志（支持文件大小切片轮转，默认 10MB 滚动，可同时输出到控制台）
inline void init_async(const std::string &logfile,
                       size_t roll_size = 10 * 1024 * 1024,
                       LogLevel::value level = LogLevel::value::INFO,
                       bool console = true,
                       const std::string &pattern = "") {
    std::unique_ptr<LoggerBuilder> builder(new LocalLoggerBuilder());
    LoggerManager::getInstance().setRootLogger(
        detail::build_helper(builder.get(), "root", Logger::Type::LOGGER_ASYNC, level, logfile, roll_size, console, pattern));
}

// 4. 多模块专用：一行创建并全局注册独立异步日志器
inline Logger::ptr create_async(const std::string &name,
                                const std::string &logfile = "",
                                size_t roll_size = 10 * 1024 * 1024,
                                LogLevel::value level = LogLevel::value::INFO,
                                bool console = false,
                                const std::string &pattern = "") {
    std::unique_ptr<LoggerBuilder> builder(new GlobalLoggerBuilder());
    return detail::build_helper(builder.get(), name, Logger::Type::LOGGER_ASYNC, level, logfile, roll_size, console, pattern);
}

// 5. 多模块专用：一行创建并全局注册独立同步日志器
inline Logger::ptr create_sync(const std::string &name,
                               const std::string &logfile = "",
                               LogLevel::value level = LogLevel::value::INFO,
                               bool console = false,
                               const std::string &pattern = "") {
    std::unique_ptr<LoggerBuilder> builder(new GlobalLoggerBuilder());
    return detail::build_helper(builder.get(), name, Logger::Type::LOGGER_SYNC, level, logfile, 0, console, pattern);
}

// 快速获取指定日志器的极简别名
inline Logger::ptr get(const std::string &name) {
    return getLogger(name);
}

// 统一目标转换器（支持传 Logger::ptr、std::string 名字 或 const char* 字符串字面量）
inline Logger::ptr to_logger(const Logger::ptr &l) { return l; }
inline Logger::ptr to_logger(const std::string &name) { return getLogger(name); }
inline Logger::ptr to_logger(const char *name) { return getLogger(name); }

// 手动显式安全退出（可选，进程正常退出时已由 RAII 自动安全守护）
inline void shutdown() {
    LoggerManager::getInstance().shutdown();
}

#if LOG_HAS_SOURCE_LOCATION
// ==========================================
// 前端三：C++20 原生无宏函数族 (依托 std::source_location)
// ==========================================
inline void debug(const std::string &msg, const std::source_location loc = std::source_location::current()) {
    rootLogger()->debug(msg, loc);
}
inline void info(const std::string &msg, const std::source_location loc = std::source_location::current()) {
    rootLogger()->info(msg, loc);
}
inline void warn(const std::string &msg, const std::source_location loc = std::source_location::current()) {
    rootLogger()->warn(msg, loc);
}
inline void error(const std::string &msg, const std::source_location loc = std::source_location::current()) {
    rootLogger()->error(msg, loc);
}
inline void fatal(const std::string &msg, const std::source_location loc = std::source_location::current()) {
    rootLogger()->fatal(msg, loc);
}
#endif

} // namespace logger

// ==========================================
// 前端一：C 风格 printf 变参宏定义族
// ==========================================

#if LOG_CPP20_OR_LATER
// ISO C++20 标准变参宏（严格符合 C++20 __VA_OPT__ 规范）
#define LOG_DEBUG(fmt, ...) (logger::rootLogger())->debug(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO(fmt, ...)  (logger::rootLogger())->info(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARN(fmt, ...)  (logger::rootLogger())->warn(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(fmt, ...) (logger::rootLogger())->error(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_FATAL(fmt, ...) (logger::rootLogger())->fatal(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)

#define LOG_DEBUG_TO(l, fmt, ...) (logger::to_logger(l))->debug(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO_TO(l, fmt, ...)  (logger::to_logger(l))->info(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARN_TO(l, fmt, ...)  (logger::to_logger(l))->warn(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR_TO(l, fmt, ...) (logger::to_logger(l))->error(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG_FATAL_TO(l, fmt, ...) (logger::to_logger(l))->fatal(__FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)
#else
// C++17 及更低版本编译器扩展（GNU/MSVC 广泛支持的 ##__VA_ARGS__）
#define LOG_DEBUG(fmt, ...) (logger::rootLogger())->debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  (logger::rootLogger())->info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  (logger::rootLogger())->warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) (logger::rootLogger())->error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) (logger::rootLogger())->fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_DEBUG_TO(l, fmt, ...) (logger::to_logger(l))->debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO_TO(l, fmt, ...)  (logger::to_logger(l))->info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN_TO(l, fmt, ...)  (logger::to_logger(l))->warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR_TO(l, fmt, ...) (logger::to_logger(l))->error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL_TO(l, fmt, ...) (logger::to_logger(l))->fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#endif

// 极简简写宏别名
#define LOGD(fmt, ...) LOG_DEBUG(fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG_INFO(fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARN(fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG_ERROR(fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) LOG_FATAL(fmt, ##__VA_ARGS__)

// ==========================================
// 前端二：C++ 现代流式 << 宏定义族（支持等级短路）
// ==========================================

// 1. 全局根日志器流式宏
#define LOG_STREAM_DEBUG LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::DEBUG)
#define LOG_STREAM_INFO  LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::INFO)
#define LOG_STREAM_WARN  LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::WARN)
#define LOG_STREAM_ERROR LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::ERROR)
#define LOG_STREAM_FATAL LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::FATAL)

// 兼容别名
#define LOG_DEBUG_S LOG_STREAM_DEBUG
#define LOG_INFO_S  LOG_STREAM_INFO
#define LOG_WARN_S  LOG_STREAM_WARN
#define LOG_ERROR_S LOG_STREAM_ERROR
#define LOG_FATAL_S LOG_STREAM_FATAL

// 2. 指定特定日志器的流式宏（同样支持传指针或直接传模块名如 "db"）
#define LOG_STREAM_DEBUG_TO(l) LOG_STREAM(logger::to_logger(l), logger::LogLevel::value::DEBUG)
#define LOG_STREAM_INFO_TO(l)  LOG_STREAM(logger::to_logger(l), logger::LogLevel::value::INFO)
#define LOG_STREAM_WARN_TO(l)  LOG_STREAM(logger::to_logger(l), logger::LogLevel::value::WARN)
#define LOG_STREAM_ERROR_TO(l) LOG_STREAM(logger::to_logger(l), logger::LogLevel::value::ERROR)
#define LOG_STREAM_FATAL_TO(l) LOG_STREAM(logger::to_logger(l), logger::LogLevel::value::FATAL)

#define LOG_S_DEBUG(l) LOG_STREAM_DEBUG_TO(l)
#define LOG_S_INFO(l)  LOG_STREAM_INFO_TO(l)
#define LOG_S_WARN(l)  LOG_STREAM_WARN_TO(l)
#define LOG_S_ERROR(l) LOG_STREAM_ERROR_TO(l)
#define LOG_S_FATAL(l) LOG_STREAM_FATAL_TO(l)

#endif // LOGGER_LOG_H_
