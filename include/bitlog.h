#ifndef __BITLOG_H__
#define __BITLOG_H__

#include "config.hpp"
#include "logger.hpp"
#include "stream.hpp"

namespace bitlog {

// 获取指定名称的日志器（必须声明为 inline，防止多源文件包含时链接报错）
inline Logger::ptr getLogger(const std::string &name) {
    return LoggerManager::getInstance().getLogger(name);
}

// 获取默认根日志器（root）
inline Logger::ptr rootLogger() {
    return LoggerManager::getInstance().rootLogger();
}

#if BITLOG_HAS_SOURCE_LOCATION
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

} // namespace bitlog

// ==========================================
// 前端一：C 风格 printf 变参宏定义族
// ==========================================
#define LOG_DEBUG(logger, fmt, ...) (logger)->debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(logger, fmt, ...)  (logger)->info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(logger, fmt, ...)  (logger)->warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(logger, fmt, ...) (logger)->error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(logger, fmt, ...) (logger)->fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

// 默认使用 rootLogger 的简化变参宏
#define LOGD(fmt, ...) LOG_DEBUG(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG_INFO(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARN(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG_ERROR(bitlog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) LOG_FATAL(bitlog::rootLogger(), fmt, ##__VA_ARGS__)

// ==========================================
// 前端二：C++ 现代流式 << 宏定义族（支持等级短路）
// ==========================================
#define LOG_S_DEBUG(logger) LOG_STREAM(logger, bitlog::LogLevel::value::DEBUG)
#define LOG_S_INFO(logger)  LOG_STREAM(logger, bitlog::LogLevel::value::INFO)
#define LOG_S_WARN(logger)  LOG_STREAM(logger, bitlog::LogLevel::value::WARN)
#define LOG_S_ERROR(logger) LOG_STREAM(logger, bitlog::LogLevel::value::ERROR)
#define LOG_S_FATAL(logger) LOG_STREAM(logger, bitlog::LogLevel::value::FATAL)

// 默认使用 rootLogger 的简化流式宏
#define LOG_S(level) LOG_STREAM(bitlog::rootLogger(), level)
#define LOG_DEBUG_S  LOG_S(bitlog::LogLevel::value::DEBUG)
#define LOG_INFO_S   LOG_S(bitlog::LogLevel::value::INFO)
#define LOG_WARN_S   LOG_S(bitlog::LogLevel::value::WARN)
#define LOG_ERROR_S  LOG_S(bitlog::LogLevel::value::ERROR)
#define LOG_FATAL_S  LOG_S(bitlog::LogLevel::value::FATAL)

#endif // __BITLOG_H__
