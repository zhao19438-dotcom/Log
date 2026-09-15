#ifndef __LOG_BUFFER_HPP__
#define __LOG_BUFFER_HPP__

#include <vector>
#include <algorithm>
#include <cassert>
#include <cstring>

namespace logger {

// 默认缓冲区大小：1MB
constexpr size_t DEFAULT_BUFFER_SIZE = 1 * 1024 * 1024;
// 阈值缓冲区大小：8MB
constexpr size_t THRESHOLD_BUFFER_SIZE = 8 * 1024 * 1024;
// 每次扩容增量：1MB
constexpr size_t INCREMENT_BUFFER_SIZE = 1 * 1024 * 1024;

// 弹性双缓冲 Buffer 类
class Buffer {
public:
    Buffer(size_t capacity = DEFAULT_BUFFER_SIZE)
        : _reader_idx(0), _writer_idx(0), _v(capacity) {}

    // 向缓冲区写入数据
    void push(const char *data, size_t len) {
        ensureEnoughSize(len);
        std::memcpy(&_v[_writer_idx], data, len);
        _writer_idx += len;
    }

    // 可读数据大小
    size_t readAbleSize() const {
        return _writer_idx - _reader_idx;
    }

    // 可写剩余空间大小
    size_t writeAbleSize() const {
        return _v.size() - _writer_idx;
    }

    // 可读数据起始指针
    const char *begin() const {
        return &_v[_reader_idx];
    }

    // 移动读指针
    void moveReader(size_t len) {
        assert(len <= readAbleSize());
        _reader_idx += len;
    }

    // 重置读写指针
    void reset() {
        _reader_idx = 0;
        _writer_idx = 0;
    }

    // 与另一个缓冲区高效交换（时间复杂度 O(1) 的指针与变量交换）
    void swap(Buffer &other) noexcept {
        _v.swap(other._v);
        std::swap(_reader_idx, other._reader_idx);
        std::swap(_writer_idx, other._writer_idx);
    }

    // 缓冲区是否为空
    bool empty() const {
        return _reader_idx == _writer_idx;
    }

private:
    // 确保有足够空间写入，不足时弹性扩容
    void ensureEnoughSize(size_t len) {
        if (len <= writeAbleSize()) return;

        size_t new_size = _v.size();
        while (new_size - _writer_idx < len) {
            if (new_size < THRESHOLD_BUFFER_SIZE) {
                new_size *= 2; // 小于阈值成倍扩容
            } else {
                new_size += INCREMENT_BUFFER_SIZE; // 大于阈值线性增量扩容
            }
        }
        _v.resize(new_size);
    }

private:
    size_t _reader_idx;
    size_t _writer_idx;
    std::vector<char> _v;
};

} // namespace logger

#endif // __LOG_BUFFER_HPP__
