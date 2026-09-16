#include "test_framework.hpp"
#include "../include/util.hpp"

int main() {
    // 确保测试输出目录干净存在
    logger::util::file::create_directory("./test_run_dir");

    int ret = test_framework::TestRegistry::instance().run_all();
    return ret;
}
