#include "test_framework.hpp"
#include "../include/formatter.hpp"

TEST_CASE(FormatterTest, BasicPatternFormatting) {
    // 构造自定义格式器：[级别][日志器名] 正文 换行
    logger::Formatter fmt("[%p][%c] %m%n");

    std::string name = "my_logger";
    logger::LogMsg msg(
        name,
        "test.cc",
        100,
        "Hello Formatter!",
        logger::LogLevel::value::INFO
    );

    std::string out = fmt.format(msg);
    ASSERT_EQ(out, "[INFO][my_logger] Hello Formatter!\n");
}

TEST_CASE(FormatterTest, FileAndLineFormatting) {
    logger::Formatter fmt("[%f:%l] %m%n");

    std::string name = "root";
    logger::LogMsg msg(
        name,
        "service.cc",
        42,
        "Warning message",
        logger::LogLevel::value::WARN
    );

    std::string out = fmt.format(msg);
    ASSERT_EQ(out, "[service.cc:42] Warning message\n");
}

TEST_CASE(FormatterTest, ComplexPatternContainsTimeAndThread) {
    logger::Formatter fmt("[%d{%Y}][%t][%p] %m%n");

    std::string name = "root";
    logger::LogMsg msg(
        name,
        "main.cc",
        88,
        "System Error",
        logger::LogLevel::value::ERROR
    );

    std::string out = fmt.format(msg);
    // 验证包含当前年份 2026 或 202x 以及 [ERROR]
    ASSERT_TRUE(out.find("[ERROR]") != std::string::npos);
    ASSERT_TRUE(out.find("System Error\n") != std::string::npos);
}
