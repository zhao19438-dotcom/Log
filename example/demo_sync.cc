#include "../include/log.h"
#include <iostream>

int main() {
    std::cout << "=== 演示 1: 使用默认 root 日志器（控制台输出） ===\n";
    // 1. C 风格变参调用
    LOGI("欢迎使用日志系统！当前版本号: %s, 核心代号: %d", "v2.0.0", 2026);
    LOGW("这是一条测试警告: %s", "磁盘剩余空间不足 10%");

    // 2. C++ 现代流式接口调用
    LOG_INFO_S << "【流式】用户登录成功 - uid=" << 10086 << ", ip=" << "192.168.1.100";
    LOG_WARN_S << "【流式】检测到异常访问频次: " << 99.8 << "%";

    std::cout << "\n=== 演示 2: 使用 Builder 自定义同步日志器（同时输出到控制台与本地文件） ===\n";
    std::unique_ptr<logger::LoggerBuilder> builder(new logger::GlobalLoggerBuilder());
    builder->buildLoggerName("sync_logger");
    builder->buildLoggerLevel(logger::LogLevel::value::DEBUG);
    builder->buildLoggerType(logger::Logger::Type::LOGGER_SYNC);
    builder->buildFormatter("[%d{%H:%M:%S}][%p][%c][%f:%l] %m%n");
    // 同时挂载控制台和文件落地
    builder->buildSink<logger::StdoutSink>();
    builder->buildSink<logger::FileSink>("./logs/sync.log");
    logger::Logger::ptr l = builder->build();

    // 双前端交叉写入
    LOG_INFO(l, "自定义同步日志器初始化完成，开始写入！");
    LOG_S_DEBUG(l) << "正在扫描系统配置参数，耗时: " << 12.5 << "ms";
    LOG_S_WARN(l)  << "连接池满载预警，当前活动连接: " << 1024;
    LOG_ERROR(l, "数据库查询异常，错误码: %d, 详情: %s", 500, "Connection Timeout");

    std::cout << "\n同步演示完成！日志已同步记录到屏幕和 ./logs/sync.log 中。\n";
    return 0;
}
