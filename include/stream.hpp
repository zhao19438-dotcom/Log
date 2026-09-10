#ifndef __LOG_STREAM_HPP__
#define __LOG_STREAM_HPP__

#include "logger.hpp"
#include <sstream>

namespace logger {

// 流式消息构建器（RAII 核心：临时对象在语句分号结束时自动析构，触发落盘提交）
class StreamMessage {
public:
    StreamMessage(const Logger::ptr &logger, LogLevel::value level, const char *file, size_t line)
        : _logger(logger), _level(level), _file(file), _line(line) {}

    // 支持任意可输出类型的链式流式追加
    template <typename T>
    StreamMessage &operator<<(const T &val) {
        _ss << val;
        return *this;
    }

    // 析构函数：临时对象生命周期结束时自动交付给日志器的统一提交层
    ~StreamMessage() {
        if (_logger) {
            _logger->submit(_level, _file, _line, _ss.str());
        }
    }

private:
    Logger::ptr _logger;
    LogLevel::value _level;
    const char *_file;
    size_t _line;
    std::stringstream _ss;
};

} // namespace logger

// 防悬垂 else 的流式短路宏封装
// 如果当前日志级别不够输出，整条 << 链条直接被跳过，绝不产生多余计算与拼接开销
#define LOG_STREAM(logger_ptr, level) \
    if (!(logger_ptr) || !(logger_ptr)->shouldLog(level)) ; \
    else logger::StreamMessage((logger_ptr), (level), __FILE__, __LINE__)

#endif // __LOG_STREAM_HPP__
