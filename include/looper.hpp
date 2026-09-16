#pragma once
#ifndef LOGGER_LOOPER_HPP_
#define LOGGER_LOOPER_HPP_

#include "buffer.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>

namespace logger {

// 异步日志工作线程与缓冲调度器
class AsyncLooper {
public:
    using ptr = std::shared_ptr<AsyncLooper>;
    using Functor = std::function<void(Buffer &buffer)>;

    // 构造函数，启动后台消费工作线程
    // @param cb 消费缓冲区数据的回调函数
    AsyncLooper(const Functor &cb)
        : _running(true), _is_processing(false), _callback(cb),
          _thread(&AsyncLooper::worker_loop, this) {}

    ~AsyncLooper() {
        stop();
    }

    // 停止工作线程并刷盘剩余日志
    void stop() {
        if (!_running.exchange(false)) return;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _pop_cond.notify_all();
            _flush_cond.notify_all();
        }
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    // 阻塞等待当前缓冲区数据全部被消费落盘
    void flush() {
        if (!_running) return;
        std::unique_lock<std::mutex> lock(_mutex);
        if (_tasks_push.empty() && !_is_processing) return;
        _pop_cond.notify_one();
        _flush_cond.wait(lock, [this]() {
            return (_tasks_push.empty() && !_is_processing) || !_running;
        });
    }

    // 将日志数据写入生产缓冲区
    // @param data 日志字节数据
    // @param len 数据长度
    void push(const char *data, size_t len) {
        if (!_running) return;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            bool need_notify = _tasks_push.empty();
            _tasks_push.push(data, len);
            if (need_notify) {
                _pop_cond.notify_one();
            }
        }
    }

private:
    // 工作线程主循环
    void worker_loop() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _pop_cond.wait(lock, [this]() {
                    return !_tasks_push.empty() || !_running;
                });

                if (!_running && _tasks_push.empty()) {
                    _flush_cond.notify_all();
                    break;
                }

                _tasks_push.swap(_tasks_pop);
                _is_processing = true;
            }

            _callback(_tasks_pop);
            _tasks_pop.reset();

            {
                std::unique_lock<std::mutex> lock(_mutex);
                _is_processing = false;
                _flush_cond.notify_all();
            }
        }
    }

private:
    std::atomic<bool> _running;
    bool _is_processing;
    Functor _callback;
    std::mutex _mutex;
    std::condition_variable _pop_cond;
    std::condition_variable _flush_cond;

    Buffer _tasks_push; // 生产缓冲区
    Buffer _tasks_pop;  // 消费缓冲区
    std::thread _thread;
};

} // namespace logger

#endif // LOGGER_LOOPER_HPP_
