#include "../include/log.h"

int main() {
    // 1. 默认控制台直写（开箱即用，适合轻量工具或单线程程序）
    LOG_INFO("同步日志系统启动成功，当前模式: 控制台直写");
    LOG_STREAM_WARN << "【流式】警告信息: 配置文件未指定，加载默认项";

    // 2. 极简 1 行开启同步文件落地（同时输出到控制台与目标文件）
    logger::init_sync("./logs/sync.log");
    LOG_INFO("全局同步落地已就绪，当前日志已同步追加至 ./logs/sync.log");

    // 3. 多模块独立输出（按模块名直写到对应模块日志文件）
    logger::create_sync("db", "./logs/sync_db.log");
    LOG_INFO_TO("db", "数据库连接池初始化成功: min_size=%d, max_size=%d", 5, 20);
    LOG_STREAM_ERROR_TO("db") << "事务提交超时，自动执行回滚！";

    return 0;
}
