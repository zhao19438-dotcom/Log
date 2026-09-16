#pragma once
#ifndef LOGGER_STREAM_HPP_
#define LOGGER_STREAM_HPP_

#include "logger.hpp"
#include <sstream>

namespace logger {

// 流式日志临时对象，析构时提交日志
class StreamMessage {
public:
    StreamMessage(const Logger::ptr &logger, LogLevel::value level, const char *file, size_t line)
        : _logger(logger), _level(level), _file(file), _line(line) {}

    // 流式输出运算符重载
    template <typename T>
    StreamMessage &operator<<(const T &val) {
        _ss << val;
        return *this;
    }

    // 析构时将拼接好的消息提交到日志器
    ~StreamMessage() {
        if (_logger) {
            _logger->submit(_level, _file, _line, _ss.str());
            if (_level == LogLevel::value::FATAL) {
                _logger->flush();
                std::abort();
            }
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

// 流式日志宏：检查级别门槛，满足时构造 StreamMessage
#define LOG_STREAM(logger_expr, level) \
    if (const auto _stream_l = (logger_expr); !_stream_l || !_stream_l->shouldLog(level)) ; \
    else logger::StreamMessage(_stream_l, (level), __FILE__, __LINE__)

#endif // LOGGER_STREAM_HPP_
