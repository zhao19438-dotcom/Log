#include "../include/log.h"
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>

int main() {
    std::cout << "=== 演示 3: 高性能异步日志器（双缓冲模式 + 自动滚动切片） ===\n";

    std::unique_ptr<logger::LoggerBuilder> builder(new logger::GlobalLoggerBuilder());
    builder->buildLoggerName("async_logger");
    builder->buildLoggerLevel(logger::LogLevel::value::INFO); // 设置过滤级别为 INFO
    builder->buildLoggerType(logger::Logger::Type::LOGGER_ASYNC);
    builder->buildFormatter("[%d{%Y-%m-%d %H:%M:%S}][%t][%p][%c] %m%n");
    builder->buildSink<logger::StdoutSink>();
    // 单个文件最大 1MB，超出自动滚动按时间切分
    builder->buildSink<logger::RollSink>("./logs/async_roll", 1024 * 1024);
    logger::Logger::ptr async_logger = builder->build();

    std::cout << "异步日志器创建成功，正在测试等级过滤机制（DEBUG 应被短路拦截）...\n";
    // 这条 DEBUG 日志将被完全拦截，且流式拼接短路
    LOG_S_DEBUG(async_logger) << "这条 DEBUG 日志不应该出现！耗时计算: " << 100 / 1;

    std::cout << "启动 3 个工作线程并发写入异步日志...\n";
    std::vector<std::thread> workers;
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back([i, async_logger]() {
            for (int j = 0; j < 5; ++j) {
                // 变参写入
                LOG_INFO(async_logger, "Worker 线程 [%d] 执行任务阶段 A, 轮次: %d", i, j);
                // 流式写入
                LOG_S_WARN(async_logger) << "Worker 线程 [" << i << "] 执行任务阶段 B - 耗时: " 
                                         << (j * 1.5) << "ms";
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }

    for (auto &w : workers) {
        w.join();
    }

    std::cout << "\n所有工作线程执行完毕！等待后台异步刷盘线程优雅退出...\n";
    return 0;
}
