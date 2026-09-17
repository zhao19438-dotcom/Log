# Log: 高性能 C++20 同步与异步日志系统技术手册

---

## 1. 系统概述

**Log** 是一个基于 ISO C++20 标准构建的轻量级、高吞吐、低延迟日志系统（向下兼容 C++11/14/17）。系统采用纯头文件（Header-Only）架构，开箱即用。核心设计应用了单例模式、建造者模式、工厂模式、策略模式与代理模式，结合操作系统多线程同步与双缓冲区机制，提供非阻塞的高并发异步日志写入能力。

### 1.1 功能特性

* **多级别日志过滤**：支持 `DEBUG`、`INFO`、`WARN`、`ERROR`、`FATAL`、`OFF` 六个等级，支持编译期条件短路与运行期动态原子调节（`std::atomic<LogLevel::value>`）。
* **双前端调用接口**：
  * **C 风格格式化宏**：支持类似 `printf` 占位符语法的变参输出（如 `LOG_INFO("id=%d", uid)`），C++20 下通过 `__VA_OPT__` 实现参数安全展开，自动捕获文件名与代码行号。
  * **C++ 现代流式前端**：支持通过 `operator<<` 链式拼接输出（如 `LOG_INFO_S << "user: " << name;`）。基于临时对象 RAII 机制在语句分号结束时原子提交，配合前置短路求值避免低等级未命中时的字符串拼接开销。
  * **原生函数调用（C++20）**：支持基于 `std::source_location` 的直接接口调用（如 `logger::info("message")`），在无宏环境下自动获取调用点源码定位信息。
* **双引擎工作模式**：
  * **同步日志（`SyncLogger`）**：调用线程直接完成格式化与写盘操作，适用于本地调试与轻量级单线程场景。
  * **异步日志（`AsyncLogger`）**：采用双缓冲区（Double Buffering）数据池与后台专有工作线程（`AsyncLooper`），写日志操作仅涉及内存拷贝与指针交换，解耦业务线程与磁盘 I/O。
* **高精度时间戳**：基于现代 C++ `<chrono>` 体系，实现毫秒（ms）级跨平台时间格式化。
* **灵活模式解析器（Pattern Formatter）**：内置基于状态机的模式格式化器，支持自定义 Pattern 格式串（如时间、线程 ID、级别、日志器名、源码文件、行号、消息正文等占位符）。
* **可扩展多落地策略（`LogSink`）**：
  * `StdoutSink`：标准控制台输出，具备类级别互斥保护。
  * `FileSink`：单文件常开追加写入，避免高频 `open`/`close` 带来的系统调用开销。
  * `RollSink`：基于文件大小阈值自动轮转滚动创建新文件。
* **模块化管理与全局单例**：提供 `LoggerManager` 管理器统一维护日志器生命周期，支持一行函数创建和全局按名称检索。
* **现代内存安全与线程安全**：
  * 核心对象全面采用智能指针（`std::shared_ptr` / `std::unique_ptr`）管理，杜绝裸指针与 C 动态内存分配（`malloc` / `free`）。
  * 强化临界区同步设计，修复并发边界下的条件变量时序问题，保证高并发场景下日志无丢失。

---

## 2. 运行环境与依赖说明

本项目遵循零外部第三方依赖原则，仅依赖标准 C++ 库及宿主操作系统底层 API。

* **开发与编译标准**：ISO C++20（推荐编译器参数 `-std=c++20 -O3 -pthread`）
* **向下兼容性**：向下支持 C++17 / C++14 / C++11（通过 `config.hpp` 自动进行标准分水岭探测）
* **支持平台**：
  * Linux（Ubuntu 18.04+、CentOS 7+ 等，内核版本 3.10+）
  * Windows（MinGW-W64 GCC 11+、MSVC 2019+）
* **构建工具**：GNU Make / mingw32-make

---

## 3. 核心技术栈

* **面向对象与多态设计**：抽象 `Logger`、`LogSink`、`FormatItem`、`LoggerBuilder` 等基类，实现高内聚低耦合的架构分层。
* **现代 C++ 特性应用**：
  * 移动语义（Move Semantics）与右值引用：用于日志消息传递，减少深拷贝。
  * 变参模板（Variadic Templates）与完美转发（Perfect Forwarding）：用于 `SinkFactory::create` 动态参数传递。
  * 智能指针与 RAII：全生命周期资源自动管理。
  * 强类型枚举与原子操作：`LogLevel::value` 与 `std::atomic` 保证并发状态安全。
  * C++20 `std::source_location` 与 `__VA_OPT__` 支持。
* **并发与同步原语**：`std::thread`、`std::mutex`、`std::unique_lock`、`std::condition_variable`、`std::atomic`。
* **双缓冲区数据池**：$O(1)$ 指针/容器置换（`std::swap`），缩短临界区持锁时间。

---

## 4. 工程结构与编译使用

### 4.1 目录结构

```text
bitlog_plus/
├── include/                  # 核心头文件库 (Header-Only)
│   ├── log.h                 # 统一对外门面头文件与便捷宏定义
│   ├── buffer.hpp            # 双缓冲内存池与动态扩容实现
│   ├── config.hpp            # C++ 标准探测与平台特性宏
│   ├── formatter.hpp         # 模式化日志格式化器 (Pattern Formatter)
│   ├── level.hpp             # 日志等级定义与转换工具
│   ├── logger.hpp            # 同步/异步日志器及建造者实现
│   ├── looper.hpp            # 异步双缓冲调度器与后台工作线程
│   ├── message.hpp           # 结构化日志元数据对象 (LogMsg)
│   ├── sink.hpp              # 多落地策略实现 (Stdout / File / Roll)
│   ├── stream.hpp            # C++ 流式 RAII 代理前端
│   └── util.hpp              # 文件系统与时间工具类
├── example/                  # 示例工程
│   ├── quickstart.cc         # 极简开箱即用示例
│   ├── demo_sync.cc          # 同步日志使用示例
│   ├── demo_async.cc         # 多线程异步并发写入示例
│   ├── custom_builder.cc     # 自定义 Builder 配置示例
│   └── Makefile              # 示例编译脚本
├── test/                     # 自动化测试套件
│   ├── test_framework.hpp    # 轻量单元测试框架
│   ├── test_main.cc          # 测试套件执行入口
│   ├── test_level.cc         # 日志级别单元测试
│   ├── test_util.cc          # 工具类单元测试
│   ├── test_buffer.cc        # 缓冲区读写与扩容单元测试
│   ├── test_formatter.cc     # 格式化器与 Pattern 单元测试
│   ├── test_sink.cc          # 落地策略单元测试
│   ├── test_looper.cc        # 异步调度器与刷新机制单元测试
│   ├── test_manager.cc       # 日志器管理器单元测试
│   ├── test_integration.cc   # 同步与异步端到端集成测试
│   └── Makefile              # 测试编译脚本
├── bench/                    # 性能压测模块
│   ├── bench.cc              # 百万级高并发压测程序
│   └── Makefile              # 压测编译脚本
├── BENCHMARK.md              # 性能基准压测评估报告
├── TECHNICAL_MANUAL.md       # 系统技术手册 (本文档)
├── README.md                 # 项目介绍与使用指南
└── Makefile                  # 根目录全局构建 Makefile
```

### 4.2 编译与测试命令

```bash
# 1. 编译全部目标 (示例、测试、压测)
make

# 2. 运行自动化测试套件 (包含 28 项全量单元与集成测试)
make test

# 3. 运行各个功能示例
./example/quickstart
./example/demo_sync
./example/demo_async
./example/custom_builder

# 4. 执行性能基准压测
./bench/bench

# 5. 清理构建产物与测试日志
make clean
```

---

## 5. 整体架构与工作流程

### 5.1 数据流向拓扑

```
[业务代码调用]
   │
   ├─► C 风格宏:   LOG_INFO("port=%d", 8080) ────────┐
   ├─► C++ 流式:   LOG_INFO_S << "user=" << name ────┼─► [提交层: Logger::submit()]
   └─► 原生接口:   logger::info("message") ──────────┘              │
                                                                   ▼
                                                       [模式格式化器: Formatter]
                                                                   │
                                                     ┌─────────────┴─────────────┐
                                                     ▼                           ▼
                                            [SyncLogger (同步直写)]      [AsyncLogger (异步引擎)]
                                                     │                           │
                                                     │                   [生产缓冲区 (_tasks_push)]
                                                     │                           │ swap (O(1))
                                                     │                   [消费缓冲区 (_tasks_pop)]
                                                     │                           │
                                                     └─────────────┬─────────────┘
                                                                   ▼
                                                     ┌─────────────┼─────────────┐
                                                     ▼             ▼             ▼
                                                [StdoutSink]  [FileSink]   [RollSink]
                                                 (终端输出)    (常开单文件)  (按大小滚动)
```

### 5.2 核心执行时序

1. **调用捕获**：业务代码调用宏或接口，抓取当前时间、文件名、行号、线程 ID、日志器名称与日志等级，构建 `LogMsg` 结构。
2. **文本格式化**：`Formatter` 按照初始化时配置的 Pattern 列表，依次将各项元数据和消息正文填充至预分配的字符串缓冲区。
3. **分发落盘**：
   - **同步日志**：直接加锁遍历内部持有的 `LogSink` 列表，执行系统写入操作。
   - **异步日志**：将格式化字符串追加至 `AsyncLooper` 的生产缓冲区 `_tasks_push`，若缓冲区原为空则通知后台工作线程唤醒。工作线程在持锁状态下将 `_tasks_push` 与 `_tasks_pop` 进行指针置换（`swap`），释放互斥锁后由后台线程无锁写入各 `LogSink`。

---

## 6. 核心组件设计与实现

### 6.1 结构化日志消息对象：`LogMsg` (`include/message.hpp`)

`LogMsg` 承载单条日志在管线中传递所需的所有上下文：

```cpp
struct LogMsg {
    std::chrono::system_clock::time_point _time; // 毫秒级时间戳
    LogLevel::value _level;                      // 日志级别
    std::thread::id _tid;                        // 线程 ID
    std::string _file;                           // 源文件名
    size_t _line;                                // 源码行号
    std::string _logger_name;                    // 所属日志器名称
    std::string _payload;                        // 消息正文

    LogMsg(const std::string &name, const char *file, size_t line, 
           std::string &&msg, LogLevel::value level)
        : _time(std::chrono::system_clock::now()),
          _level(level),
          _tid(std::this_thread::get_id()),
          _file(file ? file : ""),
          _line(line),
          _logger_name(name),
          _payload(std::move(msg)) {}
};
```

### 6.2 模式格式化器：`Formatter` (`include/formatter.hpp`)

格式化器通过解析 Pattern 字符串，将其转化为由抽象基类 `FormatItem` 构成的派生类执行序列。

#### 6.2.1 格式化项占位符表

| 占位符 | 说明 | 对应派生实现类 | 示例输出 |
|:---|:---|:---|:---|
| `%d{fmt}` | 时间戳（支持 `strftime` 格式，自动追加毫秒） | `TimeFormatItem` | `2026-09-17 10:30:00.123` |
| `%t` | 写入线程 ID | `ThreadIdFormatItem` | `140319692223424` |
| `%p` | 日志级别名称 | `LevelFormatItem` | `[INFO]` |
| `%c` | 日志器名称 | `LoggerNameFormatItem` | `[root]` |
| `%f` | 源代码文件名 | `FileFormatItem` | `server.cc` |
| `%l` | 源代码行号 | `LineFormatItem` | `128` |
| `%m` | 消息有效载荷正文 | `MsgFormatItem` | `Service started` |
| `%T` | 制表符 Tab (`\t`) | `TabFormatItem` | `\t` |
| `%n` | 换行符 (`\n`) | `NLineFormatItem` | `\n` |

#### 6.2.2 解析原理
初始化时使用两阶段状态机扫描 Pattern 字符串，区分普通字符与 `%` 开头的控制符。针对 `%d{...}` 提取内部自定义子时间格式；解析完毕后，生成对应的 `FormatItem::ptr` 存入 `std::vector`。格式化调用时遍历该容器，依次写入目标字符串。

### 6.3 双缓冲内存池：`Buffer` (`include/buffer.hpp`)

针对高频小内存写入，设计了基于连续向量内存空间的双缓冲数据池：

* **连续内存**：底层维护 `std::vector<char>`，利用 CPU 缓存局部性加速顺序内存复制（`memcpy`）。
* **动态几何扩容**：若单条日志长度超过当前剩余写入容量（`writeAbleSize`），缓冲区按 2 倍系数进行几何扩容；当单条日志尺寸远超 2 倍时，直接扩容至所需目标容量。
* **自适应缩容**：当写入脉冲高峰过去后，若缓冲区容量过大且实际使用量长期低于阈值，重置时自动缩容归位至默认基准大小，防止内存常驻过大。
* **$O(1)$ 指针交换**：通过 `Buffer::swap` 直接置换底层容器与读写游标，消除内存拷贝。

```cpp
void swap(Buffer &buf) {
    _v.swap(buf._v);
    std::swap(_reader_idx, buf._reader_idx);
    std::swap(_writer_idx, buf._writer_idx);
}
```

### 6.4 异步调度器与工作线程：`AsyncLooper` (`include/looper.hpp`)

`AsyncLooper` 负责生产缓冲区与消费缓冲区的生命周期与多线程同步：

```cpp
class AsyncLooper {
public:
    AsyncLooper(const Functor &cb)
        : _running(true), _is_processing(false), _callback(cb),
          _thread(&AsyncLooper::worker_loop, this) {}

    ~AsyncLooper() { stop(); }

    void stop();
    void flush();
    void push(const char *data, size_t len);

private:
    void worker_loop();

    std::atomic<bool> _running;
    bool _is_processing;
    Functor _callback;
    std::mutex _mutex;
    std::condition_variable _pop_cond;
    std::condition_variable _flush_cond;
    Buffer _tasks_push;
    Buffer _tasks_pop;
    std::thread _thread;
};
```

#### 关键同步机制说明
1. **Push 端**：业务线程持锁向 `_tasks_push` 写入数据，若推入前缓冲区为空，则触发 `_pop_cond.notify_one()` 唤醒后台工作线程。
2. **Worker 循环**：工作线程等待 `_pop_cond`，被唤醒后在互斥锁保护下执行 `_tasks_push.swap(_tasks_pop)`，并将 `_is_processing` 标记置为 `true`。随后立即释放互斥锁，在无锁状态下执行回调将 `_tasks_pop` 内容写入各个 `LogSink`。
3. **安全 Flush 与退出同步**：
   - 传统双缓冲实现中，`flush()` 容易因工作线程刚取出缓冲区、正在落盘回调但外部检查 `_tasks_push.empty()` 为真而提前返回，造成数据未落盘。
   - 本系统通过 `_is_processing` 状态变量与专门的 `_flush_cond` 条件变量，确保 `flush()` 必然等待后台工作线程完成当前批次的全部磁盘写入。
   - `stop()` 采用 `_running.exchange(false)` 原子操作，防止并发重复停止，并在析构时安全回收线程句柄（`join`）。

### 6.5 多路落地策略：`LogSink` 族 (`include/sink.hpp`)

所有落地类继承自抽象基类 `LogSink`，接口仅包含 `log(data, len)` 与 `flush()`：

* **`StdoutSink`**：标准输出落地。使用类静态互斥锁 `_stdout_mutex` 保护 `std::cout`，防止多日志器并发输出至终端时产生文本交织。
* **`FileSink`**：单文件持久化落地。在构造函数中通过 `util::file::create_directory` 自动递归创建父级目录，以 `std::ios::binary | std::ios::app` 模式常驻打开文件流句柄，内部具备独立互斥锁保护。
* **`RollSink`**：大小滚动文件落地。维护当前文件已写入字节大小计数器 `_cur_fsize`。当写入后达到或超过单文件阈值（`_max_fsize`）时，自动关闭当前流，生成以时间戳和序列号命名的全新文件并重新打开继续写入。

### 6.6 建造者与全局单例管理器：`LoggerManager` (`include/logger.hpp`)

系统提供 `LoggerBuilder`（建造者模式）解耦复杂日志器的装配逻辑：
* **`LocalLoggerBuilder`**：用于构建独立的局部日志器对象，返回智能指针，不自动注册进全局管理器。
* **`GlobalLoggerBuilder`**：构建日志器后自动将其加入全局单例 `LoggerManager` 注册表。
* **`LoggerManager`**：采用 Meyers' Singleton 局部静态变量实现，提供全局唯一的日志器注册与获取入口。内置默认根日志器（`root`），并在析构时统一遍历调用各日志器的 `flush()` 完成落盘。所有共享访问方法（`rootLogger`、`getLogger`、`addLogger`、`shutdown`）均具备内部互斥锁保护。

---

## 7. 接口使用规范

### 7.1 开箱即用与全局快捷初始化

```cpp
#include "log.h"

int main() {
    // 1. 初始化异步日志器 (同时输出到控制台与 logs/app.log，单文件 10MB 滚动)
    logger::init_async("./logs/app.log");

    // 2. C 风格格式化宏 (自动捕获源码文件名与行号)
    LOG_INFO("服务启动成功，监听端口: %d", 8080);
    LOG_WARN("内存占用率达到告警阈值: %.2f%%", 82.35);

    // 3. C++ 流式 RAII 接口 (强类型安全，支持任意可打印对象)
    LOG_STREAM_INFO << "用户登录: uid=" << 10001 << ", name=" << "admin";

    // 4. 动态调整日志门槛级别
    logger::rootLogger()->setLevel(logger::LogLevel::value::WARN);
    LOG_INFO("此条日志不会输出");
    LOG_ERROR("发生严重错误，正常输出");

    return 0;
}
```

### 7.2 模块化多日志器隔离输出

大型系统中各子模块可独立创建专属日志器并按名称直接输出：

```cpp
#include "log.h"

int main() {
    // 一行创建并注册独立模块日志器
    logger::create_async("db", "./logs/db.log");
    logger::create_async("net", "./logs/net.log");

    // 在任意代码位置通过名称定向写入
    LOG_INFO_TO("db", "SQL 查询完成: 耗时 %d ms", 12);
    LOG_WARN_TO("net", "TCP 连接重试: IP=%s", "192.168.1.100");
    LOG_STREAM_ERROR_TO("db") << "SQL 执行异常: 唯一键冲突";

    return 0;
}
```

### 7.3 高级深度定制：使用 Builder 链式装配

```cpp
#include "log.h"

int main() {
    auto builder = std::make_unique<logger::GlobalLoggerBuilder>();
    builder->buildLoggerName("custom");
    builder->buildLoggerLevel(logger::LogLevel::value::DEBUG);
    builder->buildLoggerType(logger::Logger::Type::LOGGER_ASYNC);
    builder->buildFormatter("[%d{%Y-%m-%d %H:%M:%S}][%p][%c][%f:%l] %m%n");
    builder->buildSink<logger::StdoutSink>();
    builder->buildSink<logger::RollSink>("./logs/custom_roll.log", 5 * 1024 * 1024);
    builder->build();

    LOG_INFO_TO("custom", "自定义日志器就绪");
    return 0;
}
```

---

## 8. 性能基准测试评估 (Benchmark)

### 8.1 测试环境配置

* **处理器**：AMD Ryzen 7 5800H with Radeon Graphics（Zen 3 微架构，8 核 16 线程，基频 3.2GHz，加速 4.4GHz，16MB L3 缓存）
* **系统内存**：16.0 GB DDR4 3200MHz 双通道
* **磁盘存储**：WDC PC SN730 512GB NVMe M.2 SSD（PCIe 3.0 x4）
* **操作系统**：Microsoft Windows 10 64 位 (Version 22H2 / Build 19045)
* **编译器版本**：MinGW-W64 GCC 13.1.0 (x86_64-posix-seh)
* **编译优化选项**：`-std=c++20 -O3 -Wall -g -pthread -I../include`

### 8.2 测试方法与指标

* **测试负载**：单条日志有效正文（Payload）固定为 100 字节，测试总写入量固定为 **1,000,000 条**（纯日志体约 95.37 MB）。
* **落地目标**：本地磁盘常开文件写入（`FileSink`）。
* **对比模式**：同步直写（`SyncLogger`）与双缓冲异步引擎（`AsyncLogger`）。
* **并发梯度**：1 线程、2 线程、3 线程、4 线程。
* **评价指标**：总耗时（秒）、吞吐量（QPS，条/秒）、数据吞吐带宽（MB/s）、单条均摊生产时延（$\mu s$）。

### 8.3 实测数据

| 运行模式 | 并发线程数 | 写入总量 | 总耗时 (s) | 吞吐量 (QPS) | 数据带宽 (MB/s) | 单条均摊生产时延 ($\mu s$) |
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

### 8.4 架构性能分析

1. **同步模式的并发逆退瓶颈**：
   在同步直写模式下，线程数由 2 增至 4 时，吞吐量从 52.8 万 QPS 下滑至 42.2 万 QPS（下降约 20%）。主要原因在于多个业务线程频繁竞争同一个底层文件锁，大部分 CPU 周期耗费在线程上下文切换（Context Switch）与系统调用陷入上。
2. **双缓冲异步模式的多核水平扩展**：
   在异步模式下，业务线程仅执行内存追加（`memcpy`）与简短的指针置换，磁盘写入完全由后台专有线程批量聚合完成。随着并发线程数增加，多核并发写入呈现近似线性扩展能力，4 线程下达到 **233.3 万+ QPS**（数据带宽 **222.56 MB/s**），相比同等同步场景吞吐量提升至 **5.5 倍**，均摊生产时延仅 **0.429 微秒**。

---

## 9. 自动化测试套件与质量保证

系统配备了轻量级、无外部依赖的自动化测试套件（位于 `test/` 目录），包含 8 大功能模块共 **28 项用例**，可通过 `make test` 一键运行：

| 测试模块 | 用例数 | 覆盖验证内容 |
| :---|:---:|:---|
| **`test_level.cc`** | 2 | 级别枚举与字符串双向转换映射、越界级别兜底行为。 |
| **`test_util.cc`** | 3 | 时间戳获取、多级路径提取、递归目录创建与存在性检测。 |
| **`test_buffer.cc`** | 5 | 初始容量、读写游标移动、指数几何扩容、峰值后自适应缩容、双缓冲快速交换。 |
| **`test_formatter.cc`** | 3 | 基础 Pattern 格式化、文件名与行号占位解析、带毫秒时间与线程 ID 复合串。 |
| **`test_sink.cc`** | 3 | `FileSink` 写入与刷新、`RollSink` 大小滚动边界轮转、`StdoutSink` 互斥保护。 |
| **`test_looper.cc`** | 2 | 异步消息推入与后台消费回调、非破坏性 `flush()` 与工作线程保活。 |
| **`test_manager.cc`** | 5 | 单例唯一性、默认 root 日志器检测、按名注册获取、重复添加异常、未找到异常。 |
| **`test_integration.cc`**| 5 | 同步全流程、高并发异步多线程零丢失、物理文件隔离、双前端混合并发、动态级别切换。 |

---

## 10. 扩展指南

得益于面向对象设计与开闭原则（OCP），扩展新的输出介质无需修改核心日志逻辑：

```cpp
#include "sink.hpp"

// 继承 LogSink 并实现抽象接口
class CustomNetworkSink : public logger::LogSink {
public:
    CustomNetworkSink(const std::string &ip, int port) : _ip(ip), _port(port) {
        // 初始化网络连接套接字
    }

    void log(const char *data, size_t len) override {
        // 通过网络套接字发送日志数据
    }

    void flush() override {
        // 刷新网络套接字发送缓冲区
    }

private:
    std::string _ip;
    int _port;
};

// 使用 Builder 挂载
builder->buildSink<CustomNetworkSink>("192.168.1.50", 9000);
```
