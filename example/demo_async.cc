#include "../include/log.h"
#include <thread>
#include <vector>

int main() {
    // 1. 初始化异步日志器，输出到 ./logs/async.log
    logger::init_async("./logs/async.log");
    LOG_INFO("异步日志器就绪");

    // 2. 创建网络模块专用异步日志器
    logger::create_async("net", "./logs/async_net.log");

    // 3. 多线程并发写入
    std::vector<std::thread> workers;
    for (int i = 1; i <= 3; ++i) {
        workers.emplace_back([i]() {
            LOG_INFO("Worker [%d] 处理业务任务", i);
            LOG_STREAM_INFO_TO("net") << "Worker [" << i << "] 上报心跳，延迟: " << (i * 2.3) << "ms";
        });
    }

    for (auto &t : workers) {
        t.join();
    }

    LOG_INFO("所有线程执行完毕");
    return 0;
}
