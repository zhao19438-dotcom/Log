#include "test_framework.hpp"
#include "../include/looper.hpp"
#include <vector>
#include <string>
#include <mutex>

TEST_CASE(LooperTest, AsyncPushAndCallback) {
    std::vector<std::string> received;
    std::mutex rec_mutex;

    {
        logger::AsyncLooper looper([&](logger::Buffer &buf) {
            std::unique_lock<std::mutex> lock(rec_mutex);
            received.emplace_back(buf.begin(), buf.readAbleSize());
        });

        std::string msg = "Async message item";
        looper.push(msg.data(), msg.size());
        looper.flush();

        {
            std::unique_lock<std::mutex> lock(rec_mutex);
            ASSERT_FALSE(received.empty());
            ASSERT_EQ(received[0], msg);
        }
    }
}

TEST_CASE(LooperTest, NonDestructiveFlushKeepsThreadAlive) {
    std::string collected = "";
    std::mutex m;

    logger::AsyncLooper looper([&](logger::Buffer &buf) {
        std::unique_lock<std::mutex> lock(m);
        collected.append(buf.begin(), buf.readAbleSize());
    });

    // 阶段 1: 写入第一阶段数据并 flush
    std::string stage1 = "STAGE_1_DATA;";
    looper.push(stage1.data(), stage1.size());
    looper.flush();

    {
        std::unique_lock<std::mutex> lock(m);
        ASSERT_EQ(collected, stage1);
    }

    // 阶段 2: 核心验证！flush 后工作线程必须保持存活，能继续正常接收后续数据
    std::string stage2 = "STAGE_2_DATA;";
    looper.push(stage2.data(), stage2.size());
    looper.flush();

    {
        std::unique_lock<std::mutex> lock(m);
        ASSERT_EQ(collected, stage1 + stage2);
    }

    looper.stop();
}
