#include "test_framework.hpp"
#include "../include/util.hpp"

TEST_CASE(UtilTest, DateNow) {
    size_t t1 = logger::util::date::now();
    ASSERT_TRUE(t1 > 1600000000); // 必须是合理的大于 2020 年的纪元秒数
}

TEST_CASE(UtilTest, FilePathExtraction) {
    ASSERT_EQ(logger::util::file::path("./logs/sub/app.log"), "./logs/sub/");
    ASSERT_EQ(logger::util::file::path("logs/app.log"), "logs/");
    ASSERT_EQ(logger::util::file::path("app.log"), ".");
    ASSERT_EQ(logger::util::file::path(""), ".");
}

TEST_CASE(UtilTest, DirectoryCreationAndExistence) {
    std::string test_dir = "./test_run_dir/nested/deep";
    logger::util::file::create_directory(test_dir);
    ASSERT_TRUE(logger::util::file::exists(test_dir));

    // 再次调用必须幂等，不报错
    ASSERT_NO_THROW(logger::util::file::create_directory(test_dir));
}
