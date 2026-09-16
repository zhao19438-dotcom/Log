#include "test_framework.hpp"
#include "../include/level.hpp"

TEST_CASE(LogLevelTest, ValidToStringMappings) {
    ASSERT_EQ(std::string(logger::LogLevel::toString(logger::LogLevel::value::DEBUG)), "DEBUG");
    ASSERT_EQ(std::string(logger::LogLevel::toString(logger::LogLevel::value::INFO)),  "INFO");
    ASSERT_EQ(std::string(logger::LogLevel::toString(logger::LogLevel::value::WARN)),  "WARN");
    ASSERT_EQ(std::string(logger::LogLevel::toString(logger::LogLevel::value::ERROR)), "ERROR");
    ASSERT_EQ(std::string(logger::LogLevel::toString(logger::LogLevel::value::FATAL)), "FATAL");
    ASSERT_EQ(std::string(logger::LogLevel::toString(logger::LogLevel::value::OFF)),   "OFF");
}

TEST_CASE(LogLevelTest, InvalidLevelFallback) {
    auto invalid_val = static_cast<logger::LogLevel::value>(999);
    ASSERT_EQ(std::string(logger::LogLevel::toString(invalid_val)), "UNKNOWN");
}
