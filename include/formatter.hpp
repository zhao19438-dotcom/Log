#pragma once
#ifndef LOGGER_FORMATTER_HPP_
#define LOGGER_FORMATTER_HPP_

#include "message.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <ctime>
#include <cassert>
#include <chrono>

namespace logger {

// 格式化子项基类
class FormatItem {
public:
    using ptr = std::shared_ptr<FormatItem>;
    virtual ~FormatItem() = default;
    virtual void format(std::string &out, const LogMsg &msg) const = 0;
};

// %m：日志正文
class MsgFormatItem : public FormatItem {
public:
    void format(std::string &out, const LogMsg &msg) const override {
        out.append(msg._payload);
    }
};

// %p：日志等级
class LevelFormatItem : public FormatItem {
public:
    void format(std::string &out, const LogMsg &msg) const override {
        out.append(LogLevel::toString(msg._level));
    }
};

// %d：日期时间戳，支持子格式如 %d{%Y-%m-%d %H:%M:%S}
class TimeFormatItem : public FormatItem {
public:
    TimeFormatItem(const std::string &fmt = "%Y-%m-%d %H:%M:%S") : _time_fmt(fmt) {
        if (_time_fmt.empty()) _time_fmt = "%Y-%m-%d %H:%M:%S";
    }
    void format(std::string &out, const LogMsg &msg) const override {
        auto time_t_now = std::chrono::system_clock::to_time_t(msg._time);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(msg._time.time_since_epoch()) % 1000;
        
        struct tm tm_time;
#ifdef _WIN32
        localtime_s(&tm_time, &time_t_now);
#else
        localtime_r(&time_t_now, &tm_time);
#endif
        std::string time_str(128, '\0');
        size_t len = strftime(&time_str[0], time_str.size(), _time_fmt.c_str(), &tm_time);
        time_str.resize(len);
        out.append(time_str);

        std::string ms_str = std::to_string(static_cast<int>(ms.count()));
        out.push_back('.');
        if (ms_str.size() < 3) {
            out.append(3 - ms_str.size(), '0');
        }
        out.append(ms_str);
    }
private:
    std::string _time_fmt; // 时间格式化子串
};

// %f：源文件名
class FileFormatItem : public FormatItem {
public:
    void format(std::string &out, const LogMsg &msg) const override {
        out.append(msg._file);
    }
};

// %l：代码行号
class LineFormatItem : public FormatItem {
public:
    void format(std::string &out, const LogMsg &msg) const override {
        out.append(std::to_string(msg._line));
    }
};

// %t：线程 ID
class ThreadFormatItem : public FormatItem {
public:
    void format(std::string &out, const LogMsg &msg) const override {
        std::ostringstream ss;
        ss << msg._tid;
        out.append(ss.str());
    }
};

// %c：日志器名称
class LoggerFormatItem : public FormatItem {
public:
    void format(std::string &out, const LogMsg &msg) const override {
        out.append(msg._logger_name);
    }
};

// %T：制表符
class TabFormatItem : public FormatItem {
public:
    void format(std::string &out, const LogMsg &) const override {
        out.append("\t");
    }
};

// %n：换行符
class NLineFormatItem : public FormatItem {
public:
    void format(std::string &out, const LogMsg &) const override {
        out.append("\n");
    }
};

// 原始普通文本
class OtherFormatItem : public FormatItem {
public:
    OtherFormatItem(const std::string &str) : _str(str) {}
    void format(std::string &out, const LogMsg &) const override {
        out.append(_str);
    }
private:
    std::string _str; // 普通文本字符串
};

// 日志格式化器
class Formatter {
public:
    using ptr = std::shared_ptr<Formatter>;

    // @param pattern 格式控制字符串，默认 "[%d{%Y-%m-%d %H:%M:%S}][%t][%p][%c][%f:%l] %m%n"
    Formatter(const std::string &pattern = "[%d{%Y-%m-%d %H:%M:%S}][%t][%p][%c][%f:%l] %m%n")
        : _pattern(pattern) {
        if (!parsePattern()) {
            std::cerr << "[Log Fatal] Formatter pattern parse failed: " << pattern << std::endl;
            std::abort();
        }
    }

    // 格式化消息并追加到输出字符串
    void format(std::string &out, const LogMsg &msg) const {
        for (const auto &item : _items) {
            item->format(out, msg);
        }
    }

    // 格式化消息并返回字符串
    std::string format(const LogMsg &msg) const {
        std::string out;
        out.reserve(256);
        format(out, msg);
        return out;
    }

    // 获取当前格式模式串
    const std::string &getPattern() const { return _pattern; }

private:
    // 解析格式字符串为各子项
    bool parsePattern() {
        std::vector<std::pair<std::string, std::string>> rules;
        std::string key;
        std::string val;
        size_t pos = 0;
        size_t len = _pattern.size();

        while (pos < len) {
            if (_pattern[pos] != '%') {
                val.push_back(_pattern[pos++]);
                continue;
            }
            if (pos + 1 < len && _pattern[pos + 1] == '%') {
                val.push_back('%');
                pos += 2;
                continue;
            }
            if (!val.empty()) {
                rules.push_back({"", val});
                val.clear();
            }
            pos++;
            if (pos >= len) {
                std::cerr << "[Log Error] % 处于末尾位置！\n";
                return false;
            }
            key = _pattern[pos++];
            if (pos < len && _pattern[pos] == '{') {
                size_t start = pos + 1;
                size_t end = _pattern.find('}', start);
                if (end == std::string::npos) {
                    std::cerr << "[Log Error] 格式串花括号未闭合！\n";
                    return false;
                }
                val = _pattern.substr(start, end - start);
                pos = end + 1;
            }
            rules.push_back({key, val});
            key.clear();
            val.clear();
        }
        if (!val.empty()) {
            rules.push_back({"", val});
        }

        for (auto &rule : rules) {
            if (rule.first.empty()) {
                _items.push_back(std::make_shared<OtherFormatItem>(rule.second));
            } else if (rule.first == "m") {
                _items.push_back(std::make_shared<MsgFormatItem>());
            } else if (rule.first == "p") {
                _items.push_back(std::make_shared<LevelFormatItem>());
            } else if (rule.first == "c") {
                _items.push_back(std::make_shared<LoggerFormatItem>());
            } else if (rule.first == "t") {
                _items.push_back(std::make_shared<ThreadFormatItem>());
            } else if (rule.first == "n") {
                _items.push_back(std::make_shared<NLineFormatItem>());
            } else if (rule.first == "d") {
                _items.push_back(std::make_shared<TimeFormatItem>(rule.second));
            } else if (rule.first == "f") {
                _items.push_back(std::make_shared<FileFormatItem>());
            } else if (rule.first == "l") {
                _items.push_back(std::make_shared<LineFormatItem>());
            } else if (rule.first == "T") {
                _items.push_back(std::make_shared<TabFormatItem>());
            } else {
                std::cerr << "[Log Warning] 未知格式化标识符: %" << rule.first << "\n";
                _items.push_back(std::make_shared<OtherFormatItem>(rule.first));
            }
        }
        return true;
    }

private:
    std::string _pattern;                // 格式化模式控制串
    std::vector<FormatItem::ptr> _items; // 格式化子项列表
};

} // namespace logger

#endif // LOGGER_FORMATTER_HPP_
