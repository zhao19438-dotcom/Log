#pragma once
#ifndef LOGGER_LEVEL_HPP_
#define LOGGER_LEVEL_HPP_

#include <string>

namespace logger {

// 日志级别定义
class LogLevel {
public:
    enum class value {
        UNKNOW = 0,
        DEBUG = 1,
        INFO,
        WARN,
        ERROR,
        FATAL,
        OFF
    };

    // 将级别枚举转换为大写字符串
    // @param level 日志等级枚举值
    // @return 返回对应的级别名称字符串
    static std::string toString(LogLevel::value level) {
        switch (level) {
            case LogLevel::value::DEBUG: return "DEBUG";
            case LogLevel::value::INFO:  return "INFO";
            case LogLevel::value::WARN:  return "WARN";
            case LogLevel::value::ERROR: return "ERROR";
            case LogLevel::value::FATAL: return "FATAL";
            case LogLevel::value::OFF:   return "OFF";
            default: return "UNKNOWN";
        }
    }
};

} // namespace logger

#endif // LOGGER_LEVEL_HPP_
