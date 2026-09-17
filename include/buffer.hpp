#pragma once
#ifndef LOGGER_BUFFER_HPP_
#define LOGGER_BUFFER_HPP_

#include <vector>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <atomic>

namespace logger {

// 默认缓冲区大小：1MB
constexpr size_t DEFAULT_BUFFER_SIZE = 1 * 1024 * 1024;
// 缩容触发阈值：8MB
constexpr size_t THRESHOLD_BUFFER_SIZE = 8 * 1024 * 1024;
// 缓冲区最大容量限制：64MB
constexpr size_t MAX_BUFFER_SIZE = 64 * 1024 * 1024;

// 连续内存缓冲区
class Buffer {
public:
    Buffer(size_t capacity = DEFAULT_BUFFER_SIZE)
        : _reader_idx(0), _writer_idx(0), _v(capacity) {}

    // 向缓冲区追加数据，空间不足时自动扩容
    // @param data 字节数据指针
    // @param len 数据长度
    // @return 追加是否成功
    bool push(const char *data, size_t len) {
        if (!ensureEnoughSize(len)) {
            _dropped_count.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        std::memcpy(&_v[_writer_idx], data, len);
        _writer_idx += len;
        return true;
    }

    // 向缓冲区追加日志字符串
    // @param msg 待追加的字符串
    // @return 追加是否成功
    bool push(const std::string &msg) {
        return push(msg.data(), msg.size());
    }

    // 可读数据字节数
    size_t readAbleSize() const {
        return _writer_idx - _reader_idx;
    }

    // 可写剩余空间字节数
    size_t writeAbleSize() const {
        return _v.size() - _writer_idx;
    }

    // 可读数据起始指针
    const char *begin() const {
        return &_v[_reader_idx];
    }

    // 移动读指针位置
    void moveReader(size_t len) {
        assert(len <= readAbleSize());
        _reader_idx += len;
    }

    // 重置读写位置，超过阈值时重置容量
    void reset() {
        _reader_idx = 0;
        _writer_idx = 0;

        if (_v.size() > THRESHOLD_BUFFER_SIZE) {
            std::vector<char> tmp(DEFAULT_BUFFER_SIZE);
            _v.swap(tmp);
        }
    }

    // 交换两个缓冲区内容
    void swap(Buffer &other) noexcept {
        _v.swap(other._v);
        std::swap(_reader_idx, other._reader_idx);
        std::swap(_writer_idx, other._writer_idx);
    }

    // 缓冲区是否为空
    bool empty() const {
        return _reader_idx == _writer_idx;
    }

    // 当前缓冲区容量
    size_t capacity() const {
        return _v.size();
    }

    // 获取超出容量被丢弃的日志条数
    static size_t droppedCount() {
        return _dropped_count.load(std::memory_order_relaxed);
    }

private:
    // 检查并按需扩容
    bool ensureEnoughSize(size_t len) {
        if (len <= writeAbleSize()) return true;

        size_t needed_size = _writer_idx + len;
        if (needed_size > MAX_BUFFER_SIZE) {
            return false;
        }

        size_t new_size = _v.size();
        while (new_size < needed_size) {
            if (new_size < THRESHOLD_BUFFER_SIZE) {
                new_size = new_size + (new_size >> 1);
            } else {
                new_size += 2 * 1024 * 1024;
            }

            if (new_size > MAX_BUFFER_SIZE) {
                new_size = MAX_BUFFER_SIZE;
                break;
            }
        }

        if (new_size < needed_size) {
            return false;
        }

        _v.resize(new_size);
        return true;
    }

private:
    size_t _reader_idx;                                 // 可读数据起始偏移量
    size_t _writer_idx;                                 // 可写数据起始偏移量
    std::vector<char> _v;                               // 缓冲区底层存储向量
    inline static std::atomic<size_t> _dropped_count{0}; // 超出容量丢弃的日志计数
};

} // namespace logger

#endif // LOGGER_BUFFER_HPP_
