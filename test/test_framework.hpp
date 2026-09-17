#pragma once
#ifndef TEST_FRAMEWORK_HPP_
#define TEST_FRAMEWORK_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <exception>

namespace test_framework {

struct TestCase {
    std::string suite_name;     // 测试套件名称
    std::string test_name;      // 测试用例名称
    std::function<void()> func; // 测试用例执行函数
};

class TestRegistry {
public:
    static TestRegistry &instance() {
        static TestRegistry reg;
        return reg;
    }

    void register_test(const std::string &suite, const std::string &name, std::function<void()> func) {
        _tests.push_back({suite, name, func});
    }

    int run_all() {
        std::cout << "\n================================================================================\n";
        std::cout << "                  Log 测试套件启动：运行全量单元测试与集成测试                  \n";
        std::cout << "================================================================================\n";

        size_t total = _tests.size();
        size_t passed = 0;
        size_t failed = 0;

        auto suite_start = std::chrono::high_resolution_clock::now();

        std::string current_suite = "";
        for (const auto &t : _tests) {
            if (t.suite_name != current_suite) {
                current_suite = t.suite_name;
                std::cout << "\n>>> [测试模块: " << current_suite << "]\n";
            }

            auto start = std::chrono::high_resolution_clock::now();
            bool ok = true;
            std::string err_msg;

            try {
                t.func();
            } catch (const std::exception &e) {
                ok = false;
                err_msg = std::string("捕获未处理的 std::exception: ") + e.what();
            } catch (...) {
                ok = false;
                err_msg = "捕获未处理的未知异常！";
            }

            auto cost = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start).count();

            if (ok) {
                std::cout << "  [PASS] " << t.test_name << " (" << cost << " us)\n";
                passed++;
            } else {
                std::cout << "  [FAIL] " << t.test_name << " (" << cost << " us)\n";
                std::cout << "         错误详情: " << err_msg << "\n";
                failed++;
            }
        }

        auto suite_cost = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - suite_start).count();

        std::cout << "\n================================================================================\n";
        std::cout << "测试汇总仪表盘 (Test Dashboard):\n";
        std::cout << "  总用例数: " << total << " | 通过: " << passed << " | 失败: " << failed 
                  << " | 总耗时: " << suite_cost << " ms\n";
        if (failed == 0) {
            std::cout << "  >>> 结论: 所有单元测试与系统集成测试全部通过！[100% SUCCESS]\n";
        } else {
            std::cout << "  >>> 结论: 存在 " << failed << " 个用例失败，请检查详细错误！[FAILED]\n";
        }
        std::cout << "================================================================================\n";

        return failed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCase> _tests; // 注册的测试用例列表
};

struct AutoRegister {
    AutoRegister(const std::string &suite, const std::string &name, std::function<void()> func) {
        TestRegistry::instance().register_test(suite, name, func);
    }
};

class TestFailureException : public std::exception {
public:
    TestFailureException(std::string msg) : _msg(std::move(msg)) {}
    const char *what() const noexcept override { return _msg.c_str(); }
private:
    std::string _msg;
};

} // namespace test_framework

#define TEST_CASE(suite, name) \
    static void test_##suite##_##name(); \
    static ::test_framework::AutoRegister reg_##suite##_##name(#suite, #name, test_##suite##_##name); \
    static void test_##suite##_##name()

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            throw ::test_framework::TestFailureException( \
                std::string("断言失败: ASSERT_TRUE(") + #expr + ") at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_FALSE(expr) \
    do { \
        if (expr) { \
            throw ::test_framework::TestFailureException( \
                std::string("断言失败: ASSERT_FALSE(") + #expr + ") at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            throw ::test_framework::TestFailureException( \
                std::string("断言相等失败: ASSERT_EQ(") + #a + ", " + #b + ") at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            throw ::test_framework::TestFailureException( \
                std::string("断言不等失败: ASSERT_NE(") + #a + ", " + #b + ") at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_THROWS(expr, ExceptionType) \
    do { \
        bool threw = false; \
        try { \
            expr; \
        } catch (const ExceptionType &) { \
            threw = true; \
        } catch (...) { \
            throw ::test_framework::TestFailureException( \
                std::string("抛出了非预期的异常类型 at ") + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
        if (!threw) { \
            throw ::test_framework::TestFailureException( \
                std::string("未捕获到预期的异常: ") + #ExceptionType + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_NO_THROW(expr) \
    do { \
        try { \
            expr; \
        } catch (const std::exception &e) { \
            throw ::test_framework::TestFailureException( \
                std::string("异常被抛出: ") + e.what() + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } catch (...) { \
            throw ::test_framework::TestFailureException( \
                std::string("未知异常被抛出 at ") + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#endif // TEST_FRAMEWORK_HPP_
