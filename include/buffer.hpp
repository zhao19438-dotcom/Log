#ifndef __LOG_BUFFER_HPP__
#define __LOG_BUFFER_HPP__

#include <vector>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <atomic>

namespace logger {

// 默认缓冲区大小：1MB
constexpr size_t DEFAULT_BUFFER_SIZE = 1 * 1024 * 1024;
// 缩容触发阈值：8MB（超过此容量且数据消费完毕后自动触发自适应缩容）
constexpr size_t THRESHOLD_BUFFER_SIZE = 8 * 1024 * 1024;
// 缓冲区硬上限保护：64MB（防止极端异常大日志或磁盘卡死导致整机 OOM 崩溃）
constexpr size_t MAX_BUFFER_SIZE = 64 * 1024 * 1024;

// 高性能弹性双缓冲 Buffer 类
class Buffer {
public:
    Buffer(size_t capacity = DEFAULT_BUFFER_SIZE)
        : _reader_idx(0), _writer_idx(0), _v(capacity) {}

    // 向缓冲区写入数据（带安全边界校验）
    bool push(const char *data, size_t len) {
        if (!ensureEnoughSize(len)) {
            // 超过最大上限，启动安全过载保护，丢弃超额数据并累计丢弃数
            _dropped_count.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        std::memcpy(&_v[_writer_idx], data, len);
        _writer_idx += len;
        return true;
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

    // 重置读写指针（附带自适应动态缩容机制）
    void reset() {
        _reader_idx = 0;
        _writer_idx = 0;

        // 【自适应缩容策略】：解决内存高水位驻留（High Water Mark Leak）
        // 如果突发流量使缓冲区扩容超过 8MB，在消费完成后自动释放多余内存归还操作系统
        if (_v.size() > THRESHOLD_BUFFER_SIZE) {
            std::vector<char> tmp(DEFAULT_BUFFER_SIZE);
            _v.swap(tmp); // 瞬时释放超大堆内存
        }
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

    // 当前缓冲区物理容量
    size_t capacity() const {
        return _v.size();
    }

    // 获取因达到上限而安全丢弃的日志计数
    static size_t droppedCount() {
        return _dropped_count.load(std::memory_order_relaxed);
    }

private:
    // 确保有足够空间写入（1.5 倍黄金比例平滑扩容 + OOM 上限防护）
    bool ensureEnoughSize(size_t len) {
        if (len <= writeAbleSize()) return true;

        size_t needed_size = _writer_idx + len;
        if (needed_size > MAX_BUFFER_SIZE) {
            return false; // 超过系统最大保护上限，拒绝扩容以防 OOM
        }

        // 采用 1.5 倍（黄金分割比例）平滑扩容，更利于内存分配器复用先前的堆内存
        size_t new_size = _v.size();
        while (new_size < needed_size) {
            if (new_size < THRESHOLD_BUFFER_SIZE) {
                // 较小规模时采用 1.5 倍扩容
                new_size = new_size + (new_size >> 1);
            } else {
                // 超过阈值后按 2MB 步长平滑递增，避免一次性分配巨额连续内存
                new_size += 2 * 1024 * 1024;
            }

            // 防止越界超过 MAX_BUFFER_SIZE
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
    size_t _reader_idx;
    size_t _writer_idx;
    std::vector<char> _v;
    inline static std::atomic<size_t> _dropped_count{0}; // 全局丢弃统计
};

} // namespace logger

#endif // __LOG_BUFFER_HPP__
