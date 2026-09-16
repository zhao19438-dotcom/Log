#pragma once
#ifndef LOGGER_CONFIG_HPP_
#define LOGGER_CONFIG_HPP_

// 获取当前 C++ 标准版本
#if defined(_MSVC_LANG)
    #define LOG_CPLUSPLUS _MSVC_LANG
#elif defined(__cplusplus)
    #define LOG_CPLUSPLUS __cplusplus
#else
    #define LOG_CPLUSPLUS 0L
#endif

// 各 C++ 标准判定宏
#define LOG_CPP11_OR_LATER (LOG_CPLUSPLUS >= 201103L)
#define LOG_CPP14_OR_LATER (LOG_CPLUSPLUS >= 201402L)
#define LOG_CPP17_OR_LATER (LOG_CPLUSPLUS >= 201703L)
#define LOG_CPP20_OR_LATER (LOG_CPLUSPLUS >= 202002L)

// 属性宏
#if LOG_CPP17_OR_LATER
    #define LOG_NODISCARD [[nodiscard]]
    #define LOG_MAYBE_UNUSED [[maybe_unused]]
    #define LOG_FALLTHROUGH [[fallthrough]]
#else
    #define LOG_NODISCARD
    #define LOG_MAYBE_UNUSED
    #define LOG_FALLTHROUGH
#endif

#include <memory>
#include <utility>
#include <string>

namespace logger {

// make_unique 兼容
#if LOG_CPP14_OR_LATER
    using std::make_unique;
#else
    template <typename T, typename... Args>
    inline std::unique_ptr<T> make_unique(Args &&...args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
#endif

// string_view 兼容
#if LOG_CPP17_OR_LATER
    #include <string_view>
    #define LOG_HAS_STRING_VIEW 1
    using string_view_t = std::string_view;
#else
    #define LOG_HAS_STRING_VIEW 0
    using string_view_t = const std::string &;
#endif

} // namespace logger

// source_location 特性检测
#if LOG_CPP20_OR_LATER && defined(__has_include)
    #if __has_include(<source_location>)
        #include <source_location>
        #if defined(__cpp_lib_source_location) || defined(_MSC_VER)
            #define LOG_HAS_SOURCE_LOCATION 1
        #endif
    #endif
#endif

#ifndef LOG_HAS_SOURCE_LOCATION
    #define LOG_HAS_SOURCE_LOCATION 0
#endif

#endif // LOGGER_CONFIG_HPP_
