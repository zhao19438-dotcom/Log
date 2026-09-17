# Log: 现代化高性能 C++ 双前端异步日志系统

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Header Only](https://img.shields.io/badge/library-Header--Only-orange.svg)](#)

Log 是一个轻量级、高性能的现代化 C++ 同步/异步日志系统，基于 ISO C++20 标准开发（向下兼容 C++11/14/17）。采用纯头文件（Header-Only）设计，开箱即用。该项目支持 C 风格格式化与现代 C++ 流式接口双前端，底层采用双缓冲（Double Buffering）机制、多落地策略扩展以及 Pattern 格式化模式解析，专为高性能服务端应用提供高吞吐、微秒级延迟的日志服务。

---

## 🌟 核心特性与架构设计

1. **双前端接口原生共存（Dual-Frontend）**：
   - **C 风格变参前端**：支持类似 `printf` 占位符风格的直接调用（如 `LOG_INFO("id=%d", uid)`）。
   - **C++ 现代流式前端**：支持通过 `operator<<` 链式拼接任意可打印类型（如 `LOG_INFO_S << "user: " << name << ", score: " << 98.5;`）。
   - **前置等级短路**：被过滤掉的低等级日志在宏调用时即被短路跳过，流式拼接零额外开销。
   - **原生函数接口**：支持 C++20 `std::source_location` 无宏调用（如 `logger::info("message")`）。
2. **高性能双缓冲异步引擎（Double Buffering）**：
   - 生产缓冲区（Push Buffer）与消费缓冲区（Pop Buffer）通过指针置换（`std::swap`，时间复杂度 $O(1)$），有效解耦业务生产线程与磁盘 I/O。
   - 业务线程执行纳秒级内存追加，后台独立工作线程批量刷盘，支持每秒百万级（1,000,000+ QPS，峰值可达 2.48M QPS）日志吞吐量。
3. **多落地策略与常开句柄（Multi-Sink Architecture）**：
   - **`StdoutSink`**：控制台标准输出，具备类静态互斥保护。
   - **`FileSink`**：常开文件流句柄追加写入，避免频繁 `open`/`close` 造成的系统调用开销。
   - **`RollSink`**：按文件大小阈值自动滚动轮转创建新文件，防止单个日志文件过度膨胀。
   - 支持单个日志器同时绑定多个 Sink（如：同时输出到终端和持久化文件）。
4. **灵活的模式格式化器（Pattern Formatter）**：
   - 支持自定义格式控制串，语法示例：`[%d{%Y-%m-%d %H:%M:%S}][%t][%p][%c][%f:%l] %m%n`。
5. **经典设计模式集成**：
   - **RAII 模式**：`StreamMessage` 临时对象在语句分号结束时自动析构并触发提交，全生命周期资源自动回收。
   - **Meyers 单例模式**：`LoggerManager` 局部静态变量保证线程安全初始化，避免全局符号冲突。
   - **建造者模式（Builder）**：提供链式调用装配日志器组件。
   - **工厂模式（Factory）**：变参模板与完美转发创建 Sink 对象。

---

## 📐 系统架构设计

```
[业务代码调用]
   │
   ├─► C 风格变参:   LOG_INFO("user: %s", name) ──────┐
   ├─► C++ 流式 RAII: LOG_INFO_S << "user: " << name ─┼─► [统一提交层: submit()]
   └─► 原生函数接口: logger::info("user: " + name) ───┘              │
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
本库为 Header-Only 设计，将 `include/` 目录加入编译包含路径，并在源文件中引入主头文件即可：

```cpp
#include "log.h"
```

### 2. 开箱即用
```cpp
#include "log.h"

int main() {
    // 1. 控制台日志输出
    LOG_INFO("系统服务启动成功，监听端口: %d", 8080);
    LOG_WARN("检测到内存占用超过阈值: %.1f%%", 85.5);
    LOG_STREAM_INFO << "用户登录: uid=" << 10001 << ", 用户名=" << "admin";

    // 2. 初始化异步日志（同时输出到控制台与滚动日志文件）
    logger::init_async("./logs/server.log");
    LOG_INFO("异步日志已启动");
    LOG_STREAM_INFO << "写入生产缓冲区，由后台工作线程批量刷盘";

    return 0;
}
```

### 3. 多模块独立日志
支持为不同业务模块（如网络模块 `net`、数据库模块 `db`）独立创建日志器并按名称定向输出：

```cpp
#include "log.h"

int main() {
    // 1. 创建并注册模块独立日志器
    logger::create_async("db", "./logs/db.log");       // 异步写数据库日志
    logger::create_async("net", "./logs/net.log");     // 异步写网络日志

    // 2. 在任意位置按名称定向输出
    LOG_INFO_TO("db", "SQL查询成功: SELECT * FROM users WHERE id = %d", 10086);
    LOG_WARN_TO("net", "客户端网络丢包重传: seq=%d", 2048);
    LOG_STREAM_ERROR_TO("db") << "SQL执行失败: errno=" << 1062;

    return 0;
}
```

### 4. 高级定制：使用 Builder 深度组装
使用 `LoggerBuilder` 支持自定义格式化串与多种输出落地端：

```cpp
#include "log.h"
#include <memory>

int main() {
    auto builder = std::make_unique<logger::GlobalLoggerBuilder>();
    builder->buildLoggerName("custom");
    builder->buildLoggerLevel(logger::LogLevel::value::DEBUG);
    builder->buildLoggerType(logger::Logger::Type::LOGGER_ASYNC);
    builder->buildFormatter("[%d{%Y-%m-%d %H:%M:%S}][%p][%c][%f:%l] %m%n");
    builder->buildSink<logger::StdoutSink>();
    builder->buildSink<logger::RollSink>("./logs/custom_roll.log", 10 * 1024 * 1024);
    builder->build();

    LOG_INFO_TO("custom", "自定义日志器就绪");
    return 0;
}
```

---

## 🛠 编译与测试

编译需要支持 C++20 的编译器（如 GCC 11+、Clang 13+ 或 MSVC 2019+）。

在 Linux / MinGW 环境下，直接在工程根目录运行：
```bash
# 1. 编译所有示例、测试与压测程序
make

# 2. 运行极简上手示例
./example/quickstart

# 3. 运行各个独立功能演示
./example/demo_sync
./example/demo_async
./example/custom_builder

# 4. 运行全模块自动化单元与集成测试套件 (28 项用例)
make test

# 5. 运行百万级日志基准性能压测
./bench/bench

# 6. 清理编译生成的目标与日志
make clean
```

---

## 📊 性能基准压测评估报告 (Benchmark Report)

针对高并发日志写入场景，在真实物理机环境下对【同步直写模式 (`SyncLogger`)】与【双缓冲异步引擎 (`AsyncLogger`)】进行 100 万条日志的基准压力测试。

### 1. 测试环境 (Test Environment)
* **CPU 处理器**：AMD Ryzen 7 5800H with Radeon Graphics (Zen 3 架构，8 核心 16 线程，基频 3.2GHz，加速最高 4.4GHz，16MB L3 三级缓存)
* **内存规格**：16.0 GB DDR4 3200MHz 双通道
* **存储设备**：WDC PC SN730 512GB 高速 NVMe M.2 SSD (PCIe 3.0 x4)
* **操作系统**：Microsoft Windows 10 家庭中文版 64 位 (Version 22H2 / Build 19045)
* **编译器与工具链**：MinGW-W64 GCC 13.1.0 (x86_64-posix-seh)
* **编译优化选项**：`-std=c++20 -O3 -Wall -g -pthread -I../include`

### 2. 测试方法 (Test Methodology)
* **载荷规格**：单条日志有效负载（Payload）固定为 100 字节，测试总写入量恒定为 **1,000,000 条**（纯日志体约 95.37 MB）。
* **落盘目标**：物理磁盘常开文件流落地（`FileSink`）。
* **对比架构**：
  * **同步直写**：业务线程直接竞争锁并同步执行格式化与磁盘 I/O 写入；
  * **双缓冲异步**：业务线程仅执行内存追加并触发双缓冲置换，由后台独占工作线程批量写盘。
* **并发梯度**：覆盖 1 线程、2 线程、3 线程、4 线程并发梯度。
* **核心衡量指标**：总耗时（秒）、系统吞吐量（QPS，条/秒）、数据吞吐带宽（MB/s）、单条均摊生产时延（$\mu s$）。

### 3. 测试结果 (Test Results)

下表为 1,000,000 条日志在各并发梯度下的实测性能数据汇总：

| 架构模式 | 线程数 | 写入总量 | 耗时(秒) | 吞吐量 (QPS) | 数据带宽 (MB/s) | 单条均摊时延 ($\mu s$) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **同步直写 (Sync)** | 1 线程 | 1,000,000 | 2.0720s | 482,625 条/秒 | 46.03 MB/s | 2.072 $\mu s$ |
| **同步直写 (Sync)** | 2 线程 | 1,000,000 | 1.8915s | 528,670 条/秒 | 50.42 MB/s | 1.892 $\mu s$ |
| **同步直写 (Sync)** | 3 线程 | 1,000,000 | 2.1710s | 460,623 条/秒 | 43.93 MB/s | 2.171 $\mu s$ |
| **同步直写 (Sync)** | 4 线程 | 1,000,000 | 2.3678s | 422,327 条/秒 | 40.28 MB/s | 2.368 $\mu s$ |
| **双缓冲异步 (Async)** | 1 线程 | 1,000,000 | 1.3209s | **757,072 条/秒** | **72.20 MB/s** | **1.321 $\mu s$** |
| **双缓冲异步 (Async)** | 2 线程 | 1,000,000 | 0.6734s | **1,485,044 条/秒** | **141.62 MB/s** | **0.673 $\mu s$** |
| **双缓冲异步 (Async)** | 3 线程 | 1,000,000 | 0.5795s | **1,725,631 条/秒** | **164.57 MB/s** | **0.579 $\mu s$** |
| **双缓冲异步 (Async)** | 4 线程 | 1,000,000 | 0.4285s | **2,333,659 条/秒** | **222.56 MB/s** | **0.429 $\mu s$** |

*注：在系统运行稳定状态下，双缓冲异步引擎最高实测可达 **2.48M+ QPS**。*

> **架构分析结论**：
> 1. **同步写入瓶颈**：随着并发线程由 2 增加至 4，由于多线程直接争用文件 I/O 锁，线程上下文切换开销放大，QPS 从 52.8 万衰减至 42.2 万（性能衰减约 20%）。
> 2. **异步引擎扩展性**：双缓冲机制通过原子指针置换解耦业务生产与磁盘 I/O。随着并发线程增加，多核吞吐呈现近似线性增长，在 4 线程时达到 **233.3 万+ QPS**（数据带宽 **222.56 MB/s**），单条生产耗时仅 **0.429 微秒**，相比同等 4 线程同步场景吞吐量提升至 **5.5 倍**。

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
