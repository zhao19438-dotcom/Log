#include "../include/log.h"

int main() {
    // 1. 默认控制台同步输出
    LOG_INFO("同步日志启动，输出到控制台");
    LOG_STREAM_WARN << "配置文件未指定，使用默认参数";

    // 2. 初始化同步文件日志器
    logger::init_sync("./logs/sync.log");
    LOG_INFO("写入同步日志文件 ./logs/sync.log");

    // 3. 创建模块同步日志器并写入
    logger::create_sync("db", "./logs/sync_db.log");
    LOG_INFO_TO("db", "数据库连接池初始化完成: min=%d, max=%d", 5, 20);
    LOG_STREAM_ERROR_TO("db") << "事务提交超时，执行回滚";

    return 0;
}
