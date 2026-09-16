#pragma once
#ifndef LOGGER_UTIL_HPP_
#define LOGGER_UTIL_HPP_

#include <iostream>
#include <string>
#include <ctime>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define LOG_MKDIR(path) _mkdir(path)
#else
#include <unistd.h>
#define LOG_MKDIR(path) mkdir(path, 0775)
#endif

namespace logger {
namespace util {

// 时间工具
class date {
public:
    // 获取当前秒级时间戳
    static size_t now() {
        return static_cast<size_t>(time(nullptr));
    }
};

// 文件与路径工具
class file {
public:
    // 判断路径是否存在
    static bool exists(const std::string &pathname) {
        struct stat st;
        return (stat(pathname.c_str(), &st) == 0);
    }

    // 从完整路径中提取目录部分（包含尾部分隔符）
    static std::string path(const std::string &pathname) {
        if (pathname.empty()) return ".";
        size_t pos = pathname.find_last_of("/\\");
        if (pos == std::string::npos) return ".";
        return pathname.substr(0, pos + 1);
    }

    // 递归创建目录
    static void create_directory(const std::string &pathname) {
        if (pathname.empty() || exists(pathname)) return;

        size_t pos = 0;
        while ((pos = pathname.find_first_of("/\\", pos)) != std::string::npos) {
            std::string sub_path = pathname.substr(0, pos);
            if (!sub_path.empty() && sub_path != "." && sub_path != "..") {
                if (!exists(sub_path)) {
                    LOG_MKDIR(sub_path.c_str());
                }
            }
            pos++;
        }
        if (!exists(pathname)) {
            LOG_MKDIR(pathname.c_str());
        }
    }
};

} // namespace util
} // namespace logger

#endif // LOGGER_UTIL_HPP_
