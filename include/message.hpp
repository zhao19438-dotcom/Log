#pragma once
#ifndef LOGGER_MESSAGE_HPP_
#define LOGGER_MESSAGE_HPP_

#include "level.hpp"
#include "util.hpp"
#include <string>
#include <thread>
#include <chrono>

namespace logger {

// 日志消息体结构
struct LogMsg {
    std::chrono::system_clock::time_point _time; // 时间戳
    LogLevel::value _level;                      // 日志级别
    std::thread::id _tid;                        // 线程 ID
    std::string _file;                           // 源文件名
    size_t _line;                                // 代码行号
    std::string _logger_name;                    // 日志器名称
    std::string _payload;                        // 日志正文

    LogMsg(const std::string &name, 
           const std::string &file, 
           size_t line, 
           std::string &&msg, 
           LogLevel::value level)
        : _time(std::chrono::system_clock::now()),
          _level(level),
          _tid(std::this_thread::get_id()),
          _file(file),
          _line(line),
          _logger_name(name),
          _payload(std::move(msg)) {}

    LogMsg(const std::string &name, 
           const char *file, 
           size_t line, 
           std::string &&msg, 
           LogLevel::value level)
        : _time(std::chrono::system_clock::now()),
          _level(level),
          _tid(std::this_thread::get_id()),
          _file(file ? file : ""),
          _line(line),
          _logger_name(name),
          _payload(std::move(msg)) {}
};

} // namespace logger

#endif // LOGGER_MESSAGE_HPP_
