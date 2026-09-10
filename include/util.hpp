#ifndef __LOG_UTIL_HPP__
#define __LOG_UTIL_HPP__

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

// 日期与时间工具类
class date {
public:
    // 获取当前时间戳（秒级）
    static size_t now() {
        return static_cast<size_t>(time(nullptr));
    }
};

// 文件与目录工具类
class file {
public:
    // 判断文件或目录是否存在
    static bool exists(const std::string &pathname) {
        struct stat st;
        return (stat(pathname.c_str(), &st) == 0);
    }

    // 从完整路径中提取文件所在目录路径（包含结尾分隔符）
    static std::string path(const std::string &pathname) {
        if (pathname.empty()) return ".";
        size_t pos = pathname.find_last_of("/\\");
        if (pos == std::string::npos) return ".";
        return pathname.substr(0, pos + 1);
    }

    // 递归逐级创建目录
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

#endif // __LOG_UTIL_HPP__
