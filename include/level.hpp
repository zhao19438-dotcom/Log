#ifndef __LOG_LEVEL_HPP__
#define __LOG_LEVEL_HPP__

#include <string>

namespace logger {

// 日志等级枚举与辅助工具
class LogLevel {
public:
    // 严格按严重程度递增排序，便于通过数值大小进行过滤 (level >= _limit)
    enum class value {
        UNKNOW = 0,
        DEBUG = 1,
        INFO,
        WARN,
        ERROR,
        FATAL,
        OFF
    };

    // 将枚举值转换为对应大写字符串
    static const char *toString(LogLevel::value level) {
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

#endif // __LOG_LEVEL_HPP__
