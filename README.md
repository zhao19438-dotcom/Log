# Log: 现代化高性能 C++ 双前端异步日志系统

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Header Only](https://img.shields.io/badge/library-Header--Only-orange.svg)](#)

Log 是一个轻量级、高性能的现代化 C++ 同步/异步日志系统。采用纯头文件（Header-Only）设计，开箱即用。该项目支持 C 风格格式化与现代 C++ 流式接口双前端，底层采用双缓冲（Double Buffering）无锁/低锁机制、多落地策略扩展以及 Pattern 格式化模式解析，专为高性能服务器与分布式应用提供高吞吐、微秒级极速日志服务。

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
#include "log.h"
```

### 2. 开箱即用（极简 1 行上手）
```cpp
#include "log.h"

int main() {
    // 1. 纯控制台日志（零配置开箱即用）
    LOG_INFO("系统服务启动成功，监听端口: %d", 8080);
    LOG_WARN("检测到内存占用超过阈值: %.1f%%", 85.5);
    LOG_STREAM_INFO << "用户登录: uid=" << 10001 << ", 用户名=" << "admin";

    // 2. 现代化一行开启高性能异步日志（同时输出到控制台 + 滚动日志文件）
    logger::init_async("./logs/server.log");
    LOG_INFO("高性能双缓冲异步引擎已启动！");
    LOG_STREAM_INFO << "像使用 printf 一样直接调用，进程退出由 RAII 自动安全刷盘！";

    return 0;
}
```

### 3. 多模块独立日志（1 行创建，直接按名字输出 ⭐）
在稍大型工程中，不同业务模块（如网络模块 `net`、数据库模块 `db`）需要分开落盘到不同文件。现在**完全不用写 Builder，也不需要满天飞传指针**：

```cpp
#include "log.h"

int main() {
    // 1. 一行创建并注册模块独立日志器
    logger::create_async("db", "./logs/db.log");       // 异步写数据库日志
    logger::create_async("net", "./logs/net.log");     // 异步写网络日志

    // 2. 在工程的任何文件、任何函数中，直接传名字打印！
    LOG_INFO_TO("db", "SQL查询成功: SELECT * FROM users WHERE id = %d", 10086);
    LOG_WARN_TO("net", "客户端网络丢包重传: seq=%d", 2048);
    LOG_STREAM_ERROR_TO("db") << "SQL执行失败: errno=" << 1062;

    return 0;
}
```

### 4. 高级定制：使用 Builder 深度组装
如果需要深度定制 Pattern、挂载 3 个以上的特殊 Sink（如后续扩展远程网络 Syslog 等），底层的 Builder 随时待命：
```cpp
#include "log.h"

int main() {
    std::unique_ptr<logger::LoggerBuilder> builder(new logger::GlobalLoggerBuilder());
    builder->buildLoggerName("custom")
           ->buildLoggerLevel(logger::LogLevel::value::DEBUG)
           ->buildLoggerType(logger::Logger::Type::LOGGER_ASYNC)
           ->buildFormatter("[%d{%Y-%m-%d %H:%M:%S}][%p][%c][%f:%l] %m%n");

    builder->buildSink<logger::StdoutSink>();
    builder->buildSink<logger::RollSink>("./logs/custom_roll", 10 * 1024 * 1024);
    auto logger = builder->build();

    LOG_INFO_TO(logger, "自定义日志器深度就绪...");
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

# 2. 运行一分钟极简上手演示
./example/quickstart

# 3. 运行独立同步直写与异步引擎演示
./example/demo_sync
./example/demo_async
./example/custom_builder

# 4. 运行全模块自动化单元与集成测试套件（28 项全绿）
make test

# 5. 运行百万级日志基准性能压测
./bench/bench

# 6. 清理编译生成的目标与日志
make clean
```

---

## 📊 性能基准压测评估报告 (Benchmark Report)

针对高并发日志写入场景，在真实物理机环境下对【同步直写模式 (`SyncLogger`)】与【双缓冲异步引擎 (`AsyncLogger`)】进行 100 万条日志的高强度基准压力测试。

### 1. 测试环境 (Test Environment)
* **CPU 处理器**：AMD Ryzen 7 5800H with Radeon Graphics (Zen 3 架构，8 核心 16 线程，基频 3.2GHz，加速最高 4.4GHz，16MB L3 三级缓存)
* **内存规格**：16.0 GB DDR4 3200MHz 双通道
* **存储设备**：WDC PC SN730 512GB 高速 NVMe M.2 SSD (PCIe 3.0 x4，高速顺序写入)
* **操作系统**：Microsoft Windows 10 家庭中文版 64 位 (Version 22H2 / Build 19045)
* **编译器与工具链**：MinGW-W64 GCC 13.1.0 (x86_64-posix-seh)
* **编译优化选项**：`-std=c++20 -O3 -Wall -g -pthread -I../include`

### 2. 测试方法 (Test Methodology)
* **载荷规格**：单条日志有效负载（Payload）固定为 100 字节，测试总写入量恒定为 **1,000,000 条**（纯日志体约 95.37 MB）。
* **落盘目标**：物理磁盘常开文件流落地（`FileSink`，消除频开频关文件句柄的系统调用损耗）。
* **对比架构**：
  * **同步直写**：业务线程直接竞争全局锁并同步执行格式化与磁盘 I/O 写入；
  * **双缓冲异步**：业务线程仅执行极速内存追加并触发微秒级双缓冲置换，由后台独占线程批量下刷磁盘。
* **并发梯度**：覆盖 1 线程、2 线程、3 线程、4 线程多核并发竞争梯次。
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

> **关键架构分析结论**：
> 1. **同步写入瓶颈**：随着并发线程由 2 增加至 4，由于多线程直接争用文件 I/O 锁，线程上下文切换开销急剧放大，QPS 从 52.8 万衰减至 42.2 万（性能恶化 ~20%）。
> 2. **异步引擎飞跃**：双缓冲机制通过原子指针置换彻底解耦业务生产与磁盘 I/O。随着并发线程增加，多核吞吐近乎线性攀升，在 4 线程时达到了 **233.3 万+ QPS**（数据带宽 **222.56 MB/s**），单条生产耗时仅 **0.429 微秒**，相比同等同步场景性能暴增 **5.5 倍**！

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
