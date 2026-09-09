# BitLog Plus: 现代化高性能 C++ 双前端异步日志系统

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Header Only](https://img.shields.io/badge/library-Header--Only-orange.svg)](#)

BitLog Plus 是一个轻量级、高性能的现代化 C++ 同步/异步日志系统。采用纯头文件（Header-Only）设计，开箱即用。该项目支持 C 风格格式化与现代 C++ 流式接口双前端，底层采用双缓冲（Double Buffering）无锁/低锁机制、多落地策略扩展以及 Pattern 格式化模式解析，专为高性能服务器与分布式应用提供高吞吐、微秒级极速日志服务。

---

## 🌟 核心特性与架构优势

1. **双前端接口原生共存（Dual-Frontend）**：
   - **C 风格变参前端**：支持类似 `printf` 占位符风格的直接调用（如 `LOG_INFO("id=%d", uid)`）。
   - **C++ 现代流式前端**：支持通过 `operator<<` 链式拼接任意可打印类型（如 `LOG_INFO_S << "user: " << name << ", score: " << 98.5;`）。
   - **前置等级短路**：被过滤掉的低等级日志在宏调用时即被短路跳过，**流式拼接零额外开销**。
2. **高性能双缓冲异步引擎（Double Buffering）**：
   - 生产缓冲区（Push Buffer）与消费缓冲区（Pop Buffer）通过指针原子级 `swap` 交换（时间复杂度 $O(1)$），彻底消除业务线程与磁盘 I/O 之间的锁竞争。
   - 业务线程微秒级写入，后台独立线程批量刷盘，支持每秒百万级（1,000,000+ QPS）日志吞吐量。
3. **多落地策略与常开句柄（Multi-Sink Architecture）**：
   - **`StdoutSink`**：控制台标准输出。
   - **`FileSink`**：常开文件流句柄追加写入，消灭频繁 open/close 造成的内核态切换与系统调用开销。
   - **`RollSink`**：按文件大小自动切片滚动轮转，防止单个日志文件无限膨胀。
   - 支持一个日志器同时绑定多个 Sink（例如：一份打屏幕，一份写本地文件）。
4. **灵活的模式格式化器（Pattern Formatter）**：
   - 支持自定义格式控制串，语法示例：`[%d{%Y-%m-%d %H:%M:%S}][%t][%p][%c][%f:%l] %m%n`。
5. **经典设计模式集成**：
   - **RAII 模式**：`StreamMessage` 临时对象在语句分号结束时自动析构并触发落盘。
   - **Meyers 单例模式**：`LoggerManager` 局部静态变量保证 C++11 线程安全，杜绝全局符号链接冲突。
   - **建造者模式（Builder）**：流畅链式装配日志器组件。
   - **工厂模式（Factory）**：模板参数完美转发。

---

## 📐 系统架构设计

```
[业务代码调用]
   │
   ├─► C 风格变参:   LOG_INFO("user: %s", name) ──────┐
   │                                                   ├──► [统一提交层: submit()]
   └─► C++ 流式 RAII: LOG_INFO_S << "user: " << name ─┘             │
                                                                    ▼
                                                         [模式解析器: Formatter]
                                                                    │
                                                      ┌─────────────┴─────────────┐
                                                      ▼                           ▼
                                             [同步直写: SyncLogger]      [异步引擎: AsyncLogger]
                                                      │                           │
                                                      │                   [生产缓冲区 (Push)]
                                                      │                           │ swap (O(1))
                                                      │                   [消费缓冲区 (Pop)]
                                                      │                           │
                                                      └─────────────┬─────────────┘
                                                                    ▼
                                                      ┌─────────────┼─────────────┐
                                                      ▼             ▼             ▼
                                                [StdoutSink]   [FileSink]    [RollSink]
                                                 (终端控制台)   (常开追加)   (按大小滚动)
```

---

## 🚀 快速上手

### 1. 引入工程
本库为 Header-Only 设计，仅需将 `include/` 目录加入你的编译包含路径，并在源文件中引入头文件：

```cpp
#include "bitlog.h"
```

### 2. 使用默认日志器（开箱即用）
```cpp
#include "bitlog.h"

int main() {
    // 变参调用
    LOGI("系统服务启动成功，监听端口: %d", 8080);
    LOGW("检测到内存占用超过阈值: %.1f%%", 85.5);

    // 流式调用
    LOG_INFO_S << "用户登录: uid=" << 10001 << ", 用户名=" << "admin";
    LOG_ERROR_S << "支付网关响应失败，错误码: " << 504;

    return 0;
}
```

### 3. 使用 Builder 组装异步多落地日志器
```cpp
#include "bitlog.h"

int main() {
    // 1. 创建全局异步日志器建造者
    std::unique_ptr<bitlog::LoggerBuilder> builder(new bitlog::GlobalLoggerBuilder());
    builder->buildLoggerName("server_logger");
    builder->buildLoggerLevel(bitlog::LogLevel::value::DEBUG);
    builder->buildLoggerType(bitlog::Logger::Type::LOGGER_ASYNC);
    builder->buildFormatter("[%d{%Y-%m-%d %H:%M:%S}][%p][%c][%f:%l] %m%n");

    // 2. 同时挂载控制台与滚动日志文件（单文件最大 10MB）
    builder->buildSink<bitlog::StdoutSink>();
    builder->buildSink<bitlog::RollSink>("./logs/server", 10 * 1024 * 1024);

    // 3. 构建并自动注册进单例管理器
    bitlog::Logger::ptr logger = builder->build();

    // 4. 使用指定日志器输出
    LOG_INFO(logger, "服务器开始监听新连接...");
    LOG_S_WARN(logger) << "客户端 [192.168.1.50] 重试次数达上限: " << 5;

    return 0;
}
```

---

## 🛠 编译与测试

编译需要支持 C++17 的编译器（如 GCC 7+、Clang 5+ 或 MSVC 2019+）。

在 Linux 环境下，直接在工程根目录运行：
```bash
# 编译所有示例与压测程序
make

# 运行同步与异步功能演示
./example/demo_sync
./example/demo_async

# 运行性能压测基准测试
./bench/bench

# 清理编译生成的目标与日志
make clean
```

---

## 📊 性能压测对比（Benchmark 参考）

在多核 Linux 服务器上对同步与双缓冲异步模式进行 100 万条日志（每条 100 字节）写入压测：

| 模式 | 线程数 | 总写入量 | 平均耗时 | 吞吐量 (QPS) | 数据吞吐率 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **同步直写 (Sync)** | 1 线程 | 1,000,000 | ~1.45 秒 | ~689,000 条/秒 | ~65.7 MB/s |
| **双缓冲异步 (Async)** | 1 线程 | 1,000,000 | ~0.58 秒 | **~1,724,000 条/秒** | **~164.4 MB/s** |
| **同步直写 (Sync)** | 4 线程 | 1,000,000 | ~2.31 秒 | ~432,000 条/秒 | ~41.2 MB/s |
| **双缓冲异步 (Async)** | 4 线程 | 1,000,000 | ~0.72 秒 | **~1,388,000 条/秒** | **~132.3 MB/s** |

> **结论**：双缓冲异步架构大幅降低了线程挂起与磁盘 I/O 阻塞，在高并发多线程写入场景下性能优势尤为显著。

---

## 📦 Git 版本管理与日常提交指南

本项目采用 Git 进行版本管理，遵循标准语义化提交规范。常用操作命令如下：

```bash
# 查看当前工作区修改状态
git status

# 将修改添加到暂存区
git add .

# 提交更新到本地版本历史
git commit -m "feat: 描述本次更新的内容"

# 推送本地提交到远程分支
git push origin main
```
