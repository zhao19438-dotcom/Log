#pragma once
#ifndef LOGGER_LOG_H_
#define LOGGER_LOG_H_

#include "config.hpp"
#include "logger.hpp"
#include "stream.hpp"

namespace logger {

// 获取指定名称的日志器
inline Logger::ptr getLogger(const std::string &name) {
    return LoggerManager::getInstance().getLogger(name);
}

// 获取默认根日志器
inline Logger::ptr rootLogger() {
    return LoggerManager::getInstance().rootLogger();
}

namespace detail {
// 构造日志器辅助函数
inline Logger::ptr build_helper(LoggerBuilder &builder,
                                const std::string &name,
                                Logger::Type type,
                                LogLevel::value level,
                                const std::string &logfile,
                                size_t roll_size,
                                bool console,
                                const std::string &pattern) {
    builder.buildLoggerName(name);
    builder.buildLoggerLevel(level);
    builder.buildLoggerType(type);
    if (!pattern.empty()) {
        builder.buildFormatter(pattern);
    }
    if (console) {
        builder.buildSink<StdoutSink>();
    }
    if (!logfile.empty()) {
        if (roll_size > 0) {
            builder.buildSink<RollSink>(logfile, roll_size);
        } else {
            builder.buildSink<FileSink>(logfile);
        }
    }
    return builder.build();
}
} // namespace detail

// 初始化控制台日志器
// @param level 日志过滤级别，默认为 INFO
// @param pattern 自定义格式化字符串，为空使用默认格式
inline void init_console(LogLevel::value level = LogLevel::value::INFO,
                         const std::string &pattern = "") {
    auto builder = std::make_unique<LocalLoggerBuilder>();
    LoggerManager::getInstance().setRootLogger(
        detail::build_helper(*builder, "root", Logger::Type::LOGGER_SYNC, level, "", 0, true, pattern));
}

// 初始化同步文件日志器
// @param logfile 日志输出文件路径
// @param level 日志过滤级别，默认为 INFO
// @param console 是否同时输出到控制台，默认 true
// @param pattern 自定义格式化字符串
inline void init_sync(const std::string &logfile,
                      LogLevel::value level = LogLevel::value::INFO,
                      bool console = true,
                      const std::string &pattern = "") {
    auto builder = std::make_unique<LocalLoggerBuilder>();
    LoggerManager::getInstance().setRootLogger(
        detail::build_helper(*builder, "root", Logger::Type::LOGGER_SYNC, level, logfile, 0, console, pattern));
}

// 初始化异步文件日志器
// @param logfile 日志输出文件路径
// @param roll_size 单文件滚动大小阈值，默认 10MB
// @param level 日志过滤级别，默认为 INFO
// @param console 是否同时输出到控制台，默认 true
// @param pattern 自定义格式化字符串
inline void init_async(const std::string &logfile,
                       size_t roll_size = 10 * 1024 * 1024,
                       LogLevel::value level = LogLevel::value::INFO,
                       bool console = true,
                       const std::string &pattern = "") {
    auto builder = std::make_unique<LocalLoggerBuilder>();
    LoggerManager::getInstance().setRootLogger(
        detail::build_helper(*builder, "root", Logger::Type::LOGGER_ASYNC, level, logfile, roll_size, console, pattern));
}

// 创建异步日志器并注册到全局管理器
// @param name 日志器名称
// @param logfile 日志输出文件路径
// @param roll_size 单文件滚动大小阈值，默认 10MB
// @param level 日志过滤级别，默认为 INFO
// @param console 是否同时输出到控制台，默认 false
// @param pattern 自定义格式化字符串
inline Logger::ptr create_async(const std::string &name,
                                const std::string &logfile = "",
                                size_t roll_size = 10 * 1024 * 1024,
                                LogLevel::value level = LogLevel::value::INFO,
                                bool console = false,
                                const std::string &pattern = "") {
    auto builder = std::make_unique<GlobalLoggerBuilder>();
    return detail::build_helper(*builder, name, Logger::Type::LOGGER_ASYNC, level, logfile, roll_size, console, pattern);
}

// 创建同步日志器并注册到全局管理器
// @param name 日志器名称
// @param logfile 日志输出文件路径
// @param level 日志过滤级别，默认为 INFO
// @param console 是否同时输出到控制台，默认 false
// @param pattern 自定义格式化字符串
inline Logger::ptr create_sync(const std::string &name,
                               const std::string &logfile = "",
                               LogLevel::value level = LogLevel::value::INFO,
                               bool console = false,
                               const std::string &pattern = "") {
    auto builder = std::make_unique<GlobalLoggerBuilder>();
    return detail::build_helper(*builder, name, Logger::Type::LOGGER_SYNC, level, logfile, 0, console, pattern);
}

// 获取指定名称日志器（简写）
inline Logger::ptr get(const std::string &name) {
    return getLogger(name);
}

// 日志目标对象转换函数
inline Logger::ptr to_logger(const Logger::ptr &l) { return l; }
inline Logger::ptr to_logger(const std::string &name) { return getLogger(name); }
inline Logger::ptr to_logger(const char *name) { return getLogger(name); }

// 显式刷新所有日志器缓冲区
inline void shutdown() {
    LoggerManager::getInstance().shutdown();
}

#if LOG_HAS_SOURCE_LOCATION
// 原生函数调用接口（基于 source_location 自动注入文件名与行号）
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

// 格式化输出宏
#if LOG_CPP20_OR_LATER
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

// 简写宏
#define LOGD(fmt, ...) LOG_DEBUG(fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG_INFO(fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARN(fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG_ERROR(fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) LOG_FATAL(fmt, ##__VA_ARGS__)

// 流式输出宏
#define LOG_STREAM_DEBUG LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::DEBUG)
#define LOG_STREAM_INFO  LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::INFO)
#define LOG_STREAM_WARN  LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::WARN)
#define LOG_STREAM_ERROR LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::ERROR)
#define LOG_STREAM_FATAL LOG_STREAM(logger::rootLogger(), logger::LogLevel::value::FATAL)

// 流式宏简写
#define LOG_DEBUG_S LOG_STREAM_DEBUG
#define LOG_INFO_S  LOG_STREAM_INFO
#define LOG_WARN_S  LOG_STREAM_WARN
#define LOG_ERROR_S LOG_STREAM_ERROR
#define LOG_FATAL_S LOG_STREAM_FATAL

// 指定日志器流式输出宏（支持日志器指针或名称）
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
