#include "../include/log.h"
#include <memory>

int main() {
    // 使用建造者模式（Builder）进行深度定制：自定义格式串与多落地目标
    auto builder = std::make_unique<logger::GlobalLoggerBuilder>();
    builder->buildLoggerName("custom");
    builder->buildLoggerLevel(logger::LogLevel::value::DEBUG);
    builder->buildLoggerType(logger::Logger::Type::LOGGER_ASYNC);
    builder->buildFormatter("[%d{%Y-%m-%d %H:%M:%S}][%p][%c][%f:%l] %m%n");
    builder->buildSink<logger::StdoutSink>();
    builder->buildSink<logger::RollSink>("./logs/custom_roll.log", 5 * 1024 * 1024);
    builder->build();

    // 全局任意位置直接按名使用
    LOG_INFO_TO("custom", "自定义格式与滚动落地就绪");
    LOG_STREAM_DEBUG_TO("custom") << "当前已启用 DEBUG 级别详细追踪";

    return 0;
}
