#pragma once
#ifndef LOGGER_STREAM_HPP_
#define LOGGER_STREAM_HPP_

#include "logger.hpp"
#include <sstream>

namespace logger {

// 流式日志临时对象，析构时提交日志
class StreamMessage {
public:
    StreamMessage(const Logger::ptr &logger, LogLevel::value level, const std::string &file, size_t line)
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
    Logger::ptr _logger;    // 目标日志器智能指针
    LogLevel::value _level; // 日志输出级别
    std::string _file;      // 源码文件名
    size_t _line;           // 源码行号
    std::stringstream _ss;  // 消息流式拼接缓冲区
};

} // namespace logger

// 流式日志宏：检查级别门槛，满足时构造 StreamMessage
#define LOG_STREAM(logger_expr, level) \
    if (const auto _stream_l = (logger_expr); !_stream_l || !_stream_l->shouldLog(level)) ; \
    else logger::StreamMessage(_stream_l, (level), __FILE__, __LINE__)

#endif // LOGGER_STREAM_HPP_
