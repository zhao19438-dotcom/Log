#include "../include/log.h"

int main() {
    // 1. 开箱即用：零配置控制台输出（支持 C 风格格式化与现代 C++ 流式双前端）
    LOG_INFO("服务启动中... 端口: %d, 版本: %s", 8080, "v2.0.0");
    LOG_STREAM_WARN << "【流式警告】检测到系统可用内存偏低: " << 18.5 << "%";

    // 2. 极简 1 行上手：启用高性能双缓冲异步引擎（同时输出到终端控制台与文件）
    logger::init_async("./logs/quickstart.log");
    LOG_INFO("异步引擎就绪，主线程非阻塞微秒级极速返回！");
    LOG_STREAM_INFO << "底层无锁双缓冲置换，无感知全自动后台落盘";

    // 3. 多模块解耦：1 行创建模块独立日志器，直接按名字路由输出
    logger::create_async("db", "./logs/quickstart_db.log");
    logger::create_async("net", "./logs/quickstart_net.log");

    LOG_INFO_TO("db", "SQL 执行成功: SELECT * FROM users WHERE id = %d", 10086);
    LOG_STREAM_WARN_TO("net") << "客户端网络波动，触发重传序列号: seq=" << 4096;
    LOG_STREAM_ERROR_TO("db") << "SQL 执行异常: 唯一键冲突 (errno: 1062)";

    // 进程正常退出时，单例 RAII 机制自动触发落盘刷盘并同步，无需手动清理
    return 0;
}
