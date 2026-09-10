#ifndef __LOG_H__
#define __LOG_H__

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
#define LOG_DEBUG(l, fmt, ...) (l)->debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(l, fmt, ...)  (l)->info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(l, fmt, ...)  (l)->warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(l, fmt, ...) (l)->error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(l, fmt, ...) (l)->fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

// 默认使用 rootLogger 的简化变参宏
#define LOGD(fmt, ...) LOG_DEBUG(logger::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG_INFO(logger::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARN(logger::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG_ERROR(logger::rootLogger(), fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) LOG_FATAL(logger::rootLogger(), fmt, ##__VA_ARGS__)

// ==========================================
// 前端二：C++ 现代流式 << 宏定义族（支持等级短路）
// ==========================================
#define LOG_S_DEBUG(l) LOG_STREAM(l, logger::LogLevel::value::DEBUG)
#define LOG_S_INFO(l)  LOG_STREAM(l, logger::LogLevel::value::INFO)
#define LOG_S_WARN(l)  LOG_STREAM(l, logger::LogLevel::value::WARN)
#define LOG_S_ERROR(l) LOG_STREAM(l, logger::LogLevel::value::ERROR)
#define LOG_S_FATAL(l) LOG_STREAM(l, logger::LogLevel::value::FATAL)

// 默认使用 rootLogger 的简化流式宏
#define LOG_S(level) LOG_STREAM(logger::rootLogger(), level)
#define LOG_DEBUG_S  LOG_S(logger::LogLevel::value::DEBUG)
#define LOG_INFO_S   LOG_S(logger::LogLevel::value::INFO)
#define LOG_WARN_S   LOG_S(logger::LogLevel::value::WARN)
#define LOG_ERROR_S  LOG_S(logger::LogLevel::value::ERROR)
#define LOG_FATAL_S  LOG_S(logger::LogLevel::value::FATAL)

#endif // __LOG_H__
