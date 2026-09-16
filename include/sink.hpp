#pragma once
#ifndef LOGGER_SINK_HPP_
#define LOGGER_SINK_HPP_

#include "util.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <cassert>
#include <mutex>

namespace logger {

// 日志落地基类
class LogSink {
public:
    using ptr = std::shared_ptr<LogSink>;
    virtual ~LogSink() = default;
    virtual void log(const char *data, size_t len) = 0;
    virtual void flush() {}
};

// 控制台标准输出落地
class StdoutSink : public LogSink {
public:
    void log(const char *data, size_t len) override {
        std::unique_lock<std::mutex> lock(_stdout_mutex);
        std::cout.write(data, len);
    }
    void flush() override {
        std::unique_lock<std::mutex> lock(_stdout_mutex);
        std::cout.flush();
    }
private:
    inline static std::mutex _stdout_mutex;
};

// 单文件追加写入落地
class FileSink : public LogSink {
public:
    // @param pathname 目标日志文件路径
    FileSink(const std::string &pathname) : _pathname(pathname) {
        util::file::create_directory(util::file::path(pathname));
        _ofs.open(pathname, std::ios::binary | std::ios::app);
        if (!_ofs.is_open()) {
            std::cerr << "[Log Fatal] 打开文件失败: " << _pathname << std::endl;
            std::abort();
        }
    }

    void log(const char *data, size_t len) override {
        std::lock_guard<std::mutex> lock(_mutex);
        _ofs.write(data, len);
        if (!_ofs.good()) {
            std::cerr << "[Log Error] 写入文件失败: " << _pathname << "\n";
        }
    }

    void flush() override {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_ofs.is_open()) {
            _ofs.flush();
        }
    }

    ~FileSink() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_ofs.is_open()) {
            _ofs.flush();
            _ofs.close();
        }
    }

private:
    std::mutex _mutex;
    std::string _pathname;
    std::ofstream _ofs;
};

// 按照文件大小滚动的落地策略
class RollSink : public LogSink {
public:
    // @param basename 基础路径前缀
    // @param max_fsize 单个文件最大字节数
    RollSink(const std::string &basename, size_t max_fsize)
        : _basename(basename), _max_fsize(max_fsize), _cur_fsize(0), _count(0) {
        util::file::create_directory(util::file::path(basename));
        std::string pathname = createNewFile();
        _ofs.open(pathname, std::ios::binary | std::ios::app);
        if (!_ofs.is_open()) {
            std::cerr << "[Log Fatal] 打开滚动文件失败: " << pathname << std::endl;
            std::abort();
        }
        _ofs.seekp(0, std::ios::end);
        _cur_fsize = static_cast<size_t>(_ofs.tellp());
    }

    void log(const char *data, size_t len) override {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_cur_fsize >= _max_fsize) {
            _ofs.close();
            std::string pathname = createNewFile();
            _ofs.open(pathname, std::ios::binary | std::ios::app);
            if (!_ofs.is_open()) {
                std::cerr << "[Log Fatal] 打开滚动文件失败: " << pathname << std::endl;
                std::abort();
            }
            _ofs.seekp(0, std::ios::end);
            _cur_fsize = static_cast<size_t>(_ofs.tellp());
        }
        _ofs.write(data, len);
        if (!_ofs.good()) {
            std::cerr << "[Log Error] 写入滚动文件失败\n";
        }
        _cur_fsize += len;
    }

    void flush() override {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_ofs.is_open()) {
            _ofs.flush();
        }
    }

    ~RollSink() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_ofs.is_open()) {
            _ofs.flush();
            _ofs.close();
        }
    }

private:
    // 生成新的滚动日志文件名
    std::string createNewFile() {
        time_t t = time(nullptr);
        struct tm tm_time;
#ifdef _WIN32
        localtime_s(&tm_time, &t);
#else
        localtime_r(&t, &tm_time);
#endif
        char time_buf[64];
        strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm_time);

        std::stringstream ss;
        ss << _basename << "_" << time_buf << "_" << (_count++) << ".log";
        return ss.str();
    }

private:
    std::mutex _mutex;
    std::string _basename;
    size_t _max_fsize;
    size_t _cur_fsize;
    size_t _count;
    std::ofstream _ofs;
};

// 落地端工厂类
class SinkFactory {
public:
    template <typename SinkType, typename ...Args>
    static LogSink::ptr create(Args &&...args) {
        return std::make_shared<SinkType>(std::forward<Args>(args)...);
    }
};

} // namespace logger

#endif // LOGGER_SINK_HPP_
