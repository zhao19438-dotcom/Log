#ifndef __LOG_LOOPER_HPP__
#define __LOG_LOOPER_HPP__

#include "buffer.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>

namespace logger {

// 异步工作线程控制器（双缓冲调度核心）
class AsyncLooper {
public:
    using ptr = std::shared_ptr<AsyncLooper>;
    using Functor = std::function<void(Buffer &buffer)>;

    AsyncLooper(const Functor &cb)
        : _running(true), _callback(cb),
          _thread(&AsyncLooper::worker_loop, this) {}

    ~AsyncLooper() {
        stop();
    }

    // 优雅停止后台线程并确保所有剩余日志落盘
    void stop() {
        if (!_running) return;
        _running = false;
        _pop_cond.notify_all();
        _flush_cond.notify_all();
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    // 非破坏性同步刷新：阻塞等待当前缓冲区所有积压数据被后台线程消费排空（工作线程保持活跃）
    void flush() {
        if (!_running) return;
        std::unique_lock<std::mutex> lock(_mutex);
        if (_tasks_push.empty() && _tasks_pop.empty()) return;
        _pop_cond.notify_one();
        _flush_cond.wait(lock, [this]() {
            return (_tasks_push.empty() && _tasks_pop.empty()) || !_running;
        });
    }

    // 业务线程调用：将日志压入生产缓冲区（微秒级）
    void push(const char *data, size_t len) {
        if (!_running) return;
        bool need_notify = false;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            need_notify = _tasks_push.empty();
            _tasks_push.push(data, len);
        }
        // 性能优化：仅在由空变非空的边界唤醒消费线程，大幅削减高频写入时的系统调用
        if (need_notify) {
            _pop_cond.notify_one();
        }
    }

private:
    // 后台专属落盘工作线程循环
    void worker_loop() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(_mutex);
                // 当处于运行状态且生产缓冲区为空时阻塞等待
                _pop_cond.wait(lock, [this]() {
                    return !_tasks_push.empty() || !_running;
                });

                // 如果已停止且生产缓冲区为空，则退出循环
                if (!_running && _tasks_push.empty()) {
                    _flush_cond.notify_all();
                    break;
                }

                // 核心关键：瞬间交换两个缓冲区（零拷贝，O(1)）
                _tasks_push.swap(_tasks_pop);
            }
            // 锁已释放！业务线程可以继续无缝写入 _tasks_push，互不干扰

            // 后台线程独占 _tasks_pop，批量刷入各落地 Sink
            _callback(_tasks_pop);
            // 重置消费缓冲区，等待下一次交换
            _tasks_pop.reset();

            // 通知可能正在阻塞等待 flush() 的线程
            {
                std::unique_lock<std::mutex> lock(_mutex);
                if (_tasks_push.empty()) {
                    _flush_cond.notify_all();
                }
            }
        }
    }

private:
    std::atomic<bool> _running;
    Functor _callback;
    std::mutex _mutex;
    std::condition_variable _pop_cond;
    std::condition_variable _flush_cond;

    Buffer _tasks_push; // 生产缓冲区（多个业务线程并发写入）
    Buffer _tasks_pop;  // 消费缓冲区（后台落盘线程独占读取）
    std::thread _thread;
};

} // namespace logger

#endif // __LOG_LOOPER_HPP__
