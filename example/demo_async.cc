#include "../include/log.h"
#include <thread>
#include <vector>

int main() {
    // 1. 极简 1 行开启高性能异步引擎（双缓冲无锁置换 + 自动切片滚动）
    logger::init_async("./logs/async.log");
    LOG_INFO("高性能异步日志系统已就绪，准备多线程并发写入...");

    // 2. 注册网络模块专用异步日志器
    logger::create_async("net", "./logs/async_net.log");

    // 3. 多工作线程并发写入验证（业务线程零 I/O 阻塞）
    std::vector<std::thread> workers;
    for (int i = 1; i <= 3; ++i) {
        workers.emplace_back([i]() {
            // C 风格变参写入主日志
            LOG_INFO("Worker 线程 [%d] 启动并处理业务任务...", i);
            // 流式接口写入模块网络日志
            LOG_STREAM_INFO_TO("net") << "Worker [" << i << "] 上报心跳数据包, 延迟: " << (i * 2.3) << "ms";
        });
    }

    for (auto &t : workers) {
        t.join();
    }

    LOG_INFO("所有工作线程任务完成！");
    // 进程退出时 RAII 自动排空异步队列并优雅落盘，无需手动 shutdown
    return 0;
}
