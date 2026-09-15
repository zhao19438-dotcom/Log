#include "../include/log.h"
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <iostream>

// 性能压测工具函数
void bench(const std::string &logger_name, size_t thread_count, size_t msg_count, size_t msg_len) {
    logger::Logger::ptr l = logger::getLogger(logger_name);
    if (!l) {
        std::cerr << "找不到日志器: " << logger_name << "\n";
        return;
    }

    std::string payload(msg_len, 'X');
    size_t count_per_thread = msg_count / thread_count;

    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "测试日志器: " << logger_name 
              << " | 线程数: " << thread_count 
              << " | 总日志数: " << msg_count 
              << " | 单条长度: " << msg_len << " 字节\n";

    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, i]() {
            for (size_t j = 0; j < count_per_thread; ++j) {
                LOG_INFO(l, "%s", payload.c_str());
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> cost = end - start;

    double total_cost = cost.count();
    double qps = msg_count / total_cost;
    double throughput = (msg_count * msg_len) / (total_cost * 1024 * 1024);

    std::cout << "总耗时: " << std::fixed << std::setprecision(4) << total_cost << " 秒\n";
    std::cout << "吞吐量 (QPS): " << std::fixed << std::setprecision(2) << qps << " 条/秒\n";
    std::cout << "数据吞吐速率: " << std::fixed << std::setprecision(2) << throughput << " MB/秒\n";
    std::cout << "------------------------------------------------------------\n";
}

int main() {
    std::cout << "============================================================\n";
    std::cout << "       Log 同步 vs 异步日志性能压测基准测试                  \n";
    std::cout << "============================================================\n";

    // 1. 创建同步测试日志器
    std::unique_ptr<logger::LoggerBuilder> sync_builder(new logger::GlobalLoggerBuilder());
    sync_builder->buildLoggerName("sync_bench");
    sync_builder->buildLoggerType(logger::Logger::Type::LOGGER_SYNC);
    sync_builder->buildFormatter("%m%n");
    sync_builder->buildSink<logger::FileSink>("./logs/sync_bench.log");
    sync_builder->build();

    // 2. 创建异步测试日志器（双缓冲模式）
    std::unique_ptr<logger::LoggerBuilder> async_builder(new logger::GlobalLoggerBuilder());
    async_builder->buildLoggerName("async_bench");
    async_builder->buildLoggerType(logger::Logger::Type::LOGGER_ASYNC);
    async_builder->buildFormatter("%m%n");
    async_builder->buildSink<logger::FileSink>("./logs/async_bench.log");
    async_builder->build();

    // 压测参数：50万条日志，单条100字节
    size_t total_msg = 500000;
    size_t msg_len = 100;

    std::cout << "\n>>> [基准 1] 单线程同步写入压测：";
    bench("sync_bench", 1, total_msg, msg_len);

    std::cout << "\n>>> [基准 2] 单线程双缓冲异步写入压测：";
    bench("async_bench", 1, total_msg, msg_len);

    std::cout << "\n>>> [基准 3] 多线程 (3 线程) 同步写入压测：";
    bench("sync_bench", 3, total_msg, msg_len);

    std::cout << "\n>>> [基准 4] 多线程 (3 线程) 双缓冲异步写入压测：";
    bench("async_bench", 3, total_msg, msg_len);

    return 0;
}
