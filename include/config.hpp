#ifndef __LOG_CONFIG_HPP__
#define __LOG_CONFIG_HPP__

// ============================================================================
// C++ 标准版本探测分水岭 (C++ Standard Feature Detection Watershed)
// ============================================================================
// MSVC 默认即使在 /std:c++17 模式下，__cplusplus 宏仍可能被固定定义为 199711L，
// 除非开启 /Zc:__cplusplus 选项。因此通过 _MSVC_LANG 宏优先获取 MSVC 的真实标准版本。
#if defined(_MSVC_LANG)
    #define LOG_CPLUSPLUS _MSVC_LANG
#elif defined(__cplusplus)
    #define LOG_CPLUSPLUS __cplusplus
#else
    #define LOG_CPLUSPLUS 0L
#endif

// 各 C++ 标准级别判定宏
#define LOG_CPP11_OR_LATER (LOG_CPLUSPLUS >= 201103L)
#define LOG_CPP14_OR_LATER (LOG_CPLUSPLUS >= 201402L)
#define LOG_CPP17_OR_LATER (LOG_CPLUSPLUS >= 201703L)
#define LOG_CPP20_OR_LATER (LOG_CPLUSPLUS >= 202002L)

// ============================================================================
// 语言属性适配宏 (Attributes Polyfill)
// ============================================================================
#if LOG_CPP17_OR_LATER
    #define LOG_NODISCARD [[nodiscard]]
    #define LOG_MAYBE_UNUSED [[maybe_unused]]
    #define LOG_FALLTHROUGH [[fallthrough]]
#else
    #define LOG_NODISCARD
    #define LOG_MAYBE_UNUSED
    #define LOG_FALLTHROUGH
#endif

// ============================================================================
// 标准库工具垫片 (Standard Library Polyfills / Fallbacks)
// ============================================================================
#include <memory>
#include <utility>
#include <string>

namespace logger {

// --- 1. make_unique 垫片 (C++14 官方引入，低版本由我们自己垫平) ---
#if LOG_CPP14_OR_LATER
    using std::make_unique;
#else
    // C++11 环境下的 make_unique 兼容实现
    template <typename T, typename... Args>
    inline std::unique_ptr<T> make_unique(Args &&...args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
#endif

// --- 2. string_view 垫片 (C++17 官方引入) ---
#if LOG_CPP17_OR_LATER
    #include <string_view>
    #define LOG_HAS_STRING_VIEW 1
    using string_view_t = std::string_view;
#else
    #define LOG_HAS_STRING_VIEW 0
    using string_view_t = const std::string &;
#endif

} // namespace logger

// ============================================================================
// C++20 前沿特性探测：std::source_location (无宏优雅获取代码位置)
// ============================================================================
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

#endif // __LOG_CONFIG_HPP__
