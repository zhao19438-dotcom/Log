#include "../include/log.h"
#include <memory>

int main() {
    // 使用 Builder 自定义格式模式串与多个输出端
    auto builder = std::make_unique<logger::GlobalLoggerBuilder>();
    builder->buildLoggerName("custom");
    builder->buildLoggerLevel(logger::LogLevel::value::DEBUG);
    builder->buildLoggerType(logger::Logger::Type::LOGGER_ASYNC);
    builder->buildFormatter("[%d{%Y-%m-%d %H:%M:%S}][%p][%c][%f:%l] %m%n");
    builder->buildSink<logger::StdoutSink>();
    builder->buildSink<logger::RollSink>("./logs/custom_roll.log", 5 * 1024 * 1024);
    builder->build();

    // 按名称使用自定义日志器
    LOG_INFO_TO("custom", "自定义日志器就绪");
    LOG_STREAM_DEBUG_TO("custom") << "当前日志级别为 DEBUG";

    return 0;
}
