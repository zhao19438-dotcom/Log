#ifndef __LOG_SINK_HPP__
#define __LOG_SINK_HPP__

#include "util.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <cassert>

namespace logger {

// 日志落地策略抽象基类
class LogSink {
public:
    using ptr = std::shared_ptr<LogSink>;
    virtual ~LogSink() = default;
    virtual void log(const char *data, size_t len) = 0;
};

// 1. 标准输出策略（终端打印）
class StdoutSink : public LogSink {
public:
    void log(const char *data, size_t len) override {
        std::cout.write(data, len);
    }
};

// 2. 固定文件落地策略（常驻文件流句柄，高性能追加）
class FileSink : public LogSink {
public:
    FileSink(const std::string &pathname) : _pathname(pathname) {
        // 创建文件所在目录
        util::file::create_directory(util::file::path(pathname));
        _ofs.open(pathname, std::ios::binary | std::ios::app);
        assert(_ofs.is_open());
    }

    void log(const char *data, size_t len) override {
        _ofs.write(data, len);
        if (!_ofs.good()) {
            std::cerr << "[Log Error] 写入文件失败: " << _pathname << "\n";
        }
    }

    ~FileSink() {
        if (_ofs.is_open()) {
            _ofs.flush();
            _ofs.close();
        }
    }

private:
    std::string _pathname;
    std::ofstream _ofs;
};

// 3. 滚动文件落地策略（按文件大小自动切片轮转）
class RollSink : public LogSink {
public:
    RollSink(const std::string &basename, size_t max_fsize)
        : _basename(basename), _max_fsize(max_fsize), _cur_fsize(0), _count(0) {
        util::file::create_directory(util::file::path(basename));
        std::string pathname = createNewFile();
        _ofs.open(pathname, std::ios::binary | std::ios::app);
        assert(_ofs.is_open());
    }

    void log(const char *data, size_t len) override {
        if (_cur_fsize >= _max_fsize) {
            _ofs.close();
            std::string pathname = createNewFile();
            _ofs.open(pathname, std::ios::binary | std::ios::app);
            assert(_ofs.is_open());
            _cur_fsize = 0;
        }
        _ofs.write(data, len);
        if (!_ofs.good()) {
            std::cerr << "[Log Error] 写入滚动文件失败\n";
        }
        _cur_fsize += len;
    }

    ~RollSink() {
        if (_ofs.is_open()) {
            _ofs.flush();
            _ofs.close();
        }
    }

private:
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
    std::string _basename;
    size_t _max_fsize;
    size_t _cur_fsize;
    size_t _count;
    std::ofstream _ofs;
};

// 落地策略模板工厂类（利用可变参数模板与完美转发）
class SinkFactory {
public:
    template <typename SinkType, typename ...Args>
    static LogSink::ptr create(Args &&...args) {
        return std::make_shared<SinkType>(std::forward<Args>(args)...);
    }
};

} // namespace logger

#endif // __LOG_SINK_HPP__
