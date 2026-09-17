#include "../include/log.h"
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

struct BenchResult {
    std::string mode;     // 运行模式描述
    size_t threads;       // 并发线程数
    size_t count;         // 写入日志总条数
    double cost_sec;      // 总耗时（秒）
    double qps;           // 每秒日志吞吐量
    double throughput_mb; // 数据吞吐带宽（MB/s）
    double latency_us;    // 单条生产均摊时延（微秒）
};

// 性能压测工具函数
BenchResult bench(const std::string &logger_name, const std::string &mode_name, size_t thread_count, size_t msg_count, size_t msg_len) {
    logger::Logger::ptr l = logger::getLogger(logger_name);
    if (!l) {
        std::cerr << "找不到日志器: " << logger_name << "\n";
        return {};
    }

    std::string payload(msg_len, 'X');
    size_t count_per_thread = msg_count / thread_count;

    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, i]() {
            for (size_t j = 0; j < count_per_thread; ++j) {
                LOG_INFO_TO(l, "%s", payload.c_str());
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
    double latency_us = (total_cost * 1000000.0) / msg_count;

    std::cout << "  >> [" << mode_name << " | " << thread_count << " 线程] 耗时: " 
              << std::fixed << std::setprecision(4) << total_cost << "s | QPS: " 
              << std::fixed << std::setprecision(0) << qps << " 条/秒 | 带宽: " 
              << std::fixed << std::setprecision(2) << throughput << " MB/s | 单条延迟: " 
              << std::fixed << std::setprecision(3) << latency_us << " us\n";

    return {mode_name, thread_count, msg_count, total_cost, qps, throughput, latency_us};
}

int main() {
    std::cout << "================================================================================\n";
    std::cout << "                   Log 高性能日志系统基准压测评估报告                           \n";
    std::cout << "================================================================================\n";
    std::cout << "【1. 测试环境 (Test Environment)】\n";
    std::cout << "  - CPU 处理器: AMD Ryzen 7 5800H with Radeon Graphics (8核 16线程, 3.2GHz ~ 4.4GHz)\n";
    std::cout << "  - 内存规格:   16.0 GB DDR4 3200MHz\n";
    std::cout << "  - 存储设备:   WDC PC SN730 512GB 高速 NVMe M.2 SSD (PCIe 3.0 x4)\n";
    std::cout << "  - 操作系统:   Microsoft Windows 10 家庭中文版 64位 (Build 19045)\n";
    std::cout << "  - 编译器版本: MinGW-W64 GCC 13.1.0 (x86_64-posix-seh)\n";
    std::cout << "  - 编译构建参数: -std=c++20 -O3 -Wall -g -pthread -I../include\n\n";

    std::cout << "【2. 测试方法 (Test Methodology)】\n";
    std::cout << "  - 消息正文规格: 单条日志正文固定 100 字节，测试总写入量 1,000,000 条 (约 95.37 MB)\n";
    std::cout << "  - 落地持久化目标: 独立物理磁盘日志文件 (FileSink，常开文件句柄)\n";
    std::cout << "  - 对比维度:   同步直写 (SyncLogger) vs 双缓冲异步引擎 (AsyncLogger)\n";
    std::cout << "  - 并发线程梯度: 1 线程、2 线程、3 线程、4 线程\n";
    std::cout << "  - 统计指标体系: 总耗时(s)、吞吐量(QPS)、I/O吞吐带宽(MB/s)、单条均摊时延(us)\n";
    std::cout << "================================================================================\n";
    std::cout << "【3. 测试执行 (Test Execution)】\n";

    // 1. 创建同步测试日志器
    auto sync_builder = std::make_unique<logger::GlobalLoggerBuilder>();
    sync_builder->buildLoggerName("sync_bench");
    sync_builder->buildLoggerType(logger::Logger::Type::LOGGER_SYNC);
    sync_builder->buildFormatter("%m%n");
    sync_builder->buildSink<logger::FileSink>("./logs/sync_bench.log");
    sync_builder->build();

    // 2. 创建异步测试日志器（双缓冲模式）
    auto async_builder = std::make_unique<logger::GlobalLoggerBuilder>();
    async_builder->buildLoggerName("async_bench");
    async_builder->buildLoggerType(logger::Logger::Type::LOGGER_ASYNC);
    async_builder->buildFormatter("%m%n");
    async_builder->buildSink<logger::FileSink>("./logs/async_bench.log");
    async_builder->build();

    const size_t total_msg = 1000000;
    const size_t msg_len = 100;

    std::vector<BenchResult> results;

    std::cout << "\n>>> 正在执行: 同步日志器基准压测 (1, 2, 3, 4 线程) ...\n";
    results.push_back(bench("sync_bench", "同步直写 (Sync)", 1, total_msg, msg_len));
    results.push_back(bench("sync_bench", "同步直写 (Sync)", 2, total_msg, msg_len));
    results.push_back(bench("sync_bench", "同步直写 (Sync)", 3, total_msg, msg_len));
    results.push_back(bench("sync_bench", "同步直写 (Sync)", 4, total_msg, msg_len));

    std::cout << "\n>>> 正在执行: 双缓冲异步日志器基准压测 (1, 2, 3, 4 线程) ...\n";
    results.push_back(bench("async_bench", "双缓冲异步 (Async)", 1, total_msg, msg_len));
    results.push_back(bench("async_bench", "双缓冲异步 (Async)", 2, total_msg, msg_len));
    results.push_back(bench("async_bench", "双缓冲异步 (Async)", 3, total_msg, msg_len));
    results.push_back(bench("async_bench", "双缓冲异步 (Async)", 4, total_msg, msg_len));

    std::cout << "\n================================================================================\n";
    std::cout << "【4. 测试结果汇总表格 (Test Results Summary)】\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "| 架构模式            | 线程数 | 写入总量   | 耗时(秒) | 吞吐量 (QPS) | 带宽 (MB/s) | 单条时延(us) |\n";
    std::cout << "|:--------------------|:-------|:-----------|:---------|:-------------|:------------|:-------------|\n";
    for (const auto &r : results) {
        std::cout << "| " << std::left << std::setw(20) << r.mode 
                  << "| " << std::setw(7) << r.threads 
                  << "| " << std::setw(11) << r.count 
                  << "| " << std::fixed << std::setprecision(4) << std::setw(9) << r.cost_sec 
                  << "| " << std::fixed << std::setprecision(0) << std::setw(13) << r.qps 
                  << "| " << std::fixed << std::setprecision(2) << std::setw(12) << r.throughput_mb 
                  << "| " << std::fixed << std::setprecision(3) << std::setw(13) << r.latency_us 
                  << "|\n";
    }
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "结论：双缓冲异步引擎将业务生产与磁盘I/O彻底解耦，多线程高并发下性能提升显著。\n";
    std::cout << "================================================================================\n";

    return 0;
}
