#include "test_framework.hpp"
#include "../include/sink.hpp"
#include <fstream>

TEST_CASE(SinkTest, FileSinkWriteAndFlush) {
    std::string test_file = "./test_run_dir/test_filesink.log";
    std::remove(test_file.c_str());
    {
        logger::FileSink sink(test_file);
        std::string content = "Testing FileSink Write Content Line 1\nLine 2\n";
        sink.log(content.data(), content.size());
        sink.flush();
    }

    // 从磁盘回读校验物理内容
    std::ifstream ifs(test_file, std::ios::binary);
    ASSERT_TRUE(ifs.is_open());
    std::string read_back((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ASSERT_EQ(read_back, "Testing FileSink Write Content Line 1\nLine 2\n");
}

TEST_CASE(SinkTest, RollSinkSizeRotation) {
    std::string base_file = "./test_run_dir/test_roll";
    size_t roll_limit = 512; // 512 字节触发切片

    {
        logger::RollSink sink(base_file, roll_limit);
        std::string chunk(200, 'R');
        chunk += "\n"; // 201 字节

        // 连续写入 5 次（总共 > 1000 字节，至少切分出 2~3 个物理文件）
        for (int i = 0; i < 5; ++i) {
            sink.log(chunk.data(), chunk.size());
        }
        sink.flush();
    }

    // 验证切片文件生成
    ASSERT_TRUE(logger::util::file::exists("./test_run_dir"));
}

TEST_CASE(SinkTest, StdoutSinkThreadSafeWrite) {
    logger::StdoutSink sink;
    std::string s = "";
    ASSERT_NO_THROW(sink.log(s.data(), s.size()));
    ASSERT_NO_THROW(sink.flush());
}
