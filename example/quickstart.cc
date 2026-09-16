#include "../include/log.h"

int main() {
    // 1. 默认控制台输出（支持 C 格式化与 C++ 流式两种写法）
    LOG_INFO("服务启动中... 端口: %d, 版本: %s", 8080, "v2.0.0");
    LOG_STREAM_WARN << "系统可用内存偏低: " << 18.5 << "%";

    // 2. 初始化异步日志器，同时输出到终端与文件
    logger::init_async("./logs/quickstart.log");
    LOG_INFO("异步日志已启动");
    LOG_STREAM_INFO << "写入生产缓冲区，由后台线程批量刷盘";

    // 3. 创建独立模块日志器，按名称指定输出
    logger::create_async("db", "./logs/quickstart_db.log");
    logger::create_async("net", "./logs/quickstart_net.log");

    LOG_INFO_TO("db", "SQL 执行成功: SELECT * FROM users WHERE id = %d", 10086);
    LOG_STREAM_WARN_TO("net") << "客户端重传数据包: seq=" << 4096;
    LOG_STREAM_ERROR_TO("db") << "SQL 执行异常: 唯一键冲突 (errno: 1062)";

    return 0;
}
