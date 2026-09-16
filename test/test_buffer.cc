#include "test_framework.hpp"
#include "../include/buffer.hpp"

TEST_CASE(BufferTest, InitialState) {
    logger::Buffer buf;
    ASSERT_TRUE(buf.empty());
    ASSERT_EQ(buf.readAbleSize(), 0);
    ASSERT_EQ(buf.writeAbleSize(), logger::DEFAULT_BUFFER_SIZE);
}

TEST_CASE(BufferTest, PushAndRead) {
    logger::Buffer buf;
    std::string msg = "Hello, High Performance Buffer!";
    buf.push(msg.data(), msg.size());

    ASSERT_FALSE(buf.empty());
    ASSERT_EQ(buf.readAbleSize(), msg.size());

    std::string read_back(buf.begin(), buf.readAbleSize());
    ASSERT_EQ(read_back, msg);

    buf.moveReader(msg.size());
    ASSERT_TRUE(buf.empty());
    ASSERT_EQ(buf.readAbleSize(), 0);
}

TEST_CASE(BufferTest, GeometricExpansion) {
    logger::Buffer buf;
    // 写入超过默认容量的数据测试扩容
    size_t big_size = 1500 * 1024;
    std::string big_payload(big_size, 'A');
    big_payload[0] = 'S';
    big_payload[big_size - 1] = 'E';

    buf.push(big_payload.data(), big_payload.size());

    ASSERT_EQ(buf.readAbleSize(), big_size);
    ASSERT_EQ(buf.begin()[0], 'S');
    ASSERT_EQ(buf.begin()[big_size - 1], 'E');
}

TEST_CASE(BufferTest, AdaptiveShrinkAfterSpike) {
    logger::Buffer buf;
    // 写入大于阈值的数据（9MB）
    size_t spike_size = 9 * 1024 * 1024;
    std::string spike_payload(spike_size, 'X');
    buf.push(spike_payload.data(), spike_payload.size());

    ASSERT_TRUE(buf.readAbleSize() == spike_size);

    // 重置缓冲区，测试容量恢复为默认大小
    buf.reset();
    ASSERT_TRUE(buf.empty());
    ASSERT_EQ(buf.writeAbleSize(), logger::DEFAULT_BUFFER_SIZE);
}

TEST_CASE(BufferTest, FastSwap) {
    logger::Buffer buf1;
    logger::Buffer buf2;

    std::string s1 = "buffer_one";
    std::string s2 = "buffer_two_extended";
    buf1.push(s1.data(), s1.size());
    buf2.push(s2.data(), s2.size());

    buf1.swap(buf2);

    ASSERT_EQ(buf1.readAbleSize(), s2.size());
    ASSERT_EQ(buf2.readAbleSize(), s1.size());

    std::string r1(buf1.begin(), buf1.readAbleSize());
    std::string r2(buf2.begin(), buf2.readAbleSize());
    ASSERT_EQ(r1, s2);
    ASSERT_EQ(r2, s1);
}
