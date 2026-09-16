#include "test_framework.hpp"
#include "../include/log.h"
#include <fstream>
#include <thread>
#include <vector>
#include <string>

TEST_CASE(IntegrationTest, FullSyncPipeline) {
    std::string sync_log = "./test_run_dir/integ_sync.log";
    std::remove(sync_log.c_str());
    auto l = logger::create_sync("integ_sync", sync_log, logger::LogLevel::value::INFO);

    LOG_DEBUG_TO(l, "This DEBUG log should be filtered!");
    LOG_INFO_TO(l, "This INFO log should be recorded: code=%d", 200);
    l->flush();

    std::ifstream ifs(sync_log);
    ASSERT_TRUE(ifs.is_open());
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ASSERT_TRUE(content.find("This DEBUG log should be filtered!") == std::string::npos);
    ASSERT_TRUE(content.find("This INFO log should be recorded: code=200") != std::string::npos);
}

TEST_CASE(IntegrationTest, HighConcurrencyAsyncZeroLoss) {
    std::string async_log = "./test_run_dir/integ_async_zero_loss.log";
    std::remove(async_log.c_str());
    auto l = logger::create_async("integ_async", async_log, 0, logger::LogLevel::value::INFO);

    const int thread_num = 10;
    const int logs_per_thread = 1000;
    const int total_expected = thread_num * logs_per_thread;

    std::vector<std::thread> workers;
    for (int t = 0; t < thread_num; ++t) {
        workers.emplace_back([=]() {
            for (int i = 0; i < logs_per_thread; ++i) {
                LOG_INFO_TO("integ_async", "[THREAD-%02d-SEQ-%04d] Concurrent Payload", t, i);
            }
        });
    }

    for (auto &w : workers) {
        w.join();
    }

    // 非破坏性刷盘排空
    l->flush();

    // 统计文件行数，必须严格等于 10000 行
    std::ifstream ifs(async_log);
    ASSERT_TRUE(ifs.is_open());
    std::string line;
    int line_count = 0;
    while (std::getline(ifs, line)) {
        if (!line.empty()) line_count++;
    }

    ASSERT_EQ(line_count, total_expected);
}

TEST_CASE(IntegrationTest, MultiModulePhysicalFileIsolation) {
    std::string net_log = "./test_run_dir/integ_net.log";
    std::string db_log  = "./test_run_dir/integ_db.log";
    std::remove(net_log.c_str());
    std::remove(db_log.c_str());

    auto net_l = logger::create_sync("integ_net", net_log, logger::LogLevel::value::DEBUG);
    auto db_l  = logger::create_sync("integ_db",  db_log,  logger::LogLevel::value::DEBUG);

    // 交叉写入两个不同模块
    for (int i = 0; i < 50; ++i) {
        LOG_INFO_TO("integ_net", "NET_PACKET_RECV id=%d", i);
        LOG_WARN_TO("integ_db",  "DB_TRANSACTION_SLOW id=%d", i);
    }

    net_l->flush();
    db_l->flush();

    // 校验 net.log: 必须只包含 NET_PACKET_RECV，绝不能出现 DB_TRANSACTION_SLOW
    std::ifstream net_ifs(net_log);
    std::string net_data((std::istreambuf_iterator<char>(net_ifs)), std::istreambuf_iterator<char>());
    ASSERT_TRUE(net_data.find("NET_PACKET_RECV") != std::string::npos);
    ASSERT_TRUE(net_data.find("DB_TRANSACTION_SLOW") == std::string::npos);

    // 校验 db.log: 必须只包含 DB_TRANSACTION_SLOW，绝不能出现 NET_PACKET_RECV
    std::ifstream db_ifs(db_log);
    std::string db_data((std::istreambuf_iterator<char>(db_ifs)), std::istreambuf_iterator<char>());
    ASSERT_TRUE(db_data.find("DB_TRANSACTION_SLOW") != std::string::npos);
    ASSERT_TRUE(db_data.find("NET_PACKET_RECV") == std::string::npos);
}

TEST_CASE(IntegrationTest, DualFrontendMixedConcurrency) {
    std::string mix_log = "./test_run_dir/integ_mixed.log";
    std::remove(mix_log.c_str());
    auto l = logger::create_sync("integ_mix", mix_log, logger::LogLevel::value::DEBUG);

    // C 风格与 C++ 流式交错并发写入同一日志器
    std::thread t1([&]() {
        for (int i = 0; i < 50; ++i) {
            LOG_INFO_TO("integ_mix", "C-Style entry %d", i);
        }
    });

    std::thread t2([&]() {
        for (int i = 0; i < 50; ++i) {
            LOG_STREAM_INFO_TO("integ_mix") << "Stream-Style entry " << i;
        }
    });

    t1.join();
    t2.join();
    l->flush();

    std::ifstream ifs(mix_log);
    std::string data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ASSERT_TRUE(data.find("C-Style entry") != std::string::npos);
    ASSERT_TRUE(data.find("Stream-Style entry") != std::string::npos);
}

TEST_CASE(IntegrationTest, DynamicLogLevelSwitching) {
    std::string dyn_log = "./test_run_dir/integ_dynamic.log";
    std::remove(dyn_log.c_str());
    auto l = logger::create_sync("integ_dyn", dyn_log, logger::LogLevel::value::WARN);

    // 当前为 WARN，DEBUG 应该被静默丢弃
    LOG_DEBUG_TO("integ_dyn", "SUPPRESSED_DEBUG_MESSAGE");
    l->flush();

    {
        std::ifstream ifs(dyn_log);
        std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ASSERT_TRUE(s.find("SUPPRESSED_DEBUG_MESSAGE") == std::string::npos);
    }

    // 运行时热切换等级为 DEBUG
    l->setLevel(logger::LogLevel::value::DEBUG);

    // 再次写入 DEBUG，必须成功记录
    LOG_DEBUG_TO("integ_dyn", "ALLOWED_DEBUG_MESSAGE");
    l->flush();

    {
        std::ifstream ifs(dyn_log);
        std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ASSERT_TRUE(s.find("ALLOWED_DEBUG_MESSAGE") != std::string::npos);
    }
}
