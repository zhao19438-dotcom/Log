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
    LOG_INFO_TO(l, "自定义同步日志器初始化完成，开始写入！");
    LOG_S_DEBUG(l) << "正在扫描系统配置参数，耗时: " << 12.5 << "ms";
    LOG_S_WARN(l)  << "连接池满载预警，当前活动连接: " << 1024;
    LOG_ERROR_TO(l, "数据库查询异常，错误码: %d, 详情: %s", 500, "Connection Timeout");

    std::cout << "\n=== 演示 3: 现代化 1 行极简初始化（全局托管 + 自动安全退出） ===\n";
    logger::init_sync("./logs/app_quick.log");
    LOG_INFO("极简 API 初始化完成！当前输出目标: 终端控制台 + ./logs/app_quick.log");
    LOG_STREAM_INFO << "像使用 printf 一样直接调用，或者丝滑流式写入，进程退出无需手动 shutdown！";

    std::cout << "\n=== 演示 4: 多模块独立日志（1 行创建，直接按模块名极简输出） ===\n";
    // 一行创建模块专用日志器
    logger::create_sync("db", "./logs/db.log");
    logger::create_sync("net", "./logs/net.log");

    // 任意业务函数中直接传入模块名，免去传递指针的烦恼！
    LOG_INFO_TO("db", "SQL查询成功: SELECT * FROM users WHERE id = %d", 10086);
    LOG_WARN_TO("net", "客户端网络波动，重传数据包: seq=%d", 4096);
    LOG_STREAM_ERROR_TO("db") << "SQL执行失败: 唯一键冲突 (errno: " << 1062 << ")";

    std::cout << "\n同步演示完成！所有日志已安全持久化至对应文件。\n";
    return 0;
}
