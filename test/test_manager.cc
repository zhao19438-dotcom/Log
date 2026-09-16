#include "test_framework.hpp"
#include "../include/logger.hpp"

TEST_CASE(ManagerTest, SingletonIdentity) {
    auto &m1 = logger::LoggerManager::getInstance();
    auto &m2 = logger::LoggerManager::getInstance();
    ASSERT_TRUE(&m1 == &m2);
}

TEST_CASE(ManagerTest, DefaultRootLoggerExists) {
    auto root = logger::LoggerManager::getInstance().rootLogger();
    ASSERT_TRUE(root != nullptr);
    ASSERT_EQ(root->loggerName(), "root");
}

TEST_CASE(ManagerTest, AddAndGetLogger) {
    auto builder = std::make_unique<logger::LocalLoggerBuilder>();
    builder->buildLoggerName("test_mgr_mod");
    builder->buildLoggerType(logger::Logger::Type::LOGGER_SYNC);
    auto l = builder->build();

    auto &mgr = logger::LoggerManager::getInstance();
    mgr.addLogger(l);

    ASSERT_TRUE(mgr.hasLogger("test_mgr_mod"));
    auto fetched = mgr.getLogger("test_mgr_mod");
    ASSERT_EQ(fetched->loggerName(), "test_mgr_mod");
}

TEST_CASE(ManagerTest, DuplicateAddThrowsException) {
    auto builder = std::make_unique<logger::LocalLoggerBuilder>();
    builder->buildLoggerName("test_mgr_mod"); // 同名
    builder->buildLoggerType(logger::Logger::Type::LOGGER_SYNC);
    auto l = builder->build();

    auto &mgr = logger::LoggerManager::getInstance();
    // 已经存在，必须抛出 std::invalid_argument 防止静默覆盖
    ASSERT_THROWS(mgr.addLogger(l), std::invalid_argument);
}

TEST_CASE(ManagerTest, GetNonExistentLoggerThrowsException) {
    auto &mgr = logger::LoggerManager::getInstance();
    // 查询不存在的名字必须抛出异常，贯彻 Fail-Fast 原则
    ASSERT_THROWS(mgr.getLogger("not_exists_logger_xyz"), std::invalid_argument);
}
