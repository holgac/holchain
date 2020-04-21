#include <gtest/gtest.h>
#include "logger.h"

class LoggerTest : public ::testing::Test {
protected:
  LoggerTest() : target_(new InMemoryLogTarget(kLogSize)) {
    logger_.addTarget(std::unique_ptr<LogTarget>(target_));
  }
  ~LoggerTest() override {}
  void SetUp() override {}
  void TearDown() override {
    // not deleting target_ since it's owned by logger
  }

  void expectSubstring(const std::string& haystack, const std::string& needle) {
    EXPECT_NE(haystack.find(needle), std::string::npos) <<
      needle << " not found in " << haystack;
  }

  const int kLogSize = 50;
  InMemoryLogTarget* target_;
  Logger logger_;
};

TEST_F(LoggerTest, LogTarget) {
  logger_.info("test");
  EXPECT_EQ(target_->getLogs().size(), 1);
}

TEST_F(LoggerTest, LogFormatting) {
  const int kLogCount = 10;
  for (int i=0; i<kLogCount; ++i) {
    logger_.info("test %d", i);
  }
  auto logs = target_->getLogs();
  ASSERT_EQ(logs.size(), kLogCount);
  for (int i=0; i<kLogCount; ++i) {
    char buf[32];
    sprintf(buf, "test %d", i);
    expectSubstring(target_->getLogs()[i], buf);
  }
}

TEST_F(LoggerTest, LogLine) {
  logger_.info() << "test" << 123;
  logger_.info() << "tast" << 125;
  auto logs = target_->getLogs();
  ASSERT_EQ(logs.size(), 2);
  expectSubstring(logs[0], "test123");
  expectSubstring(logs[1], "tast125");
}

TEST_F(LoggerTest, Errno) {
  int kErrno = 5;
  char buf[512];
  char* res;
  res = strerror_r(kErrno, buf, 512);
  errno = 5;
  logger_.logErrno() << "test" << 125;
  auto logs = target_->getLogs();
  ASSERT_EQ(logs.size(), 1);
  expectSubstring(logs[0], "test125");
  expectSubstring(logs[0], res);
}

TEST_F(LoggerTest, LogRotation) {
  const int kLogCount = kLogSize * 2;
  for (int i=0; i<kLogCount; ++i) {
    logger_.info("test %d", i);
  }
  auto logs = target_->getLogs();
  ASSERT_EQ(logs.size(), kLogSize);
  for (int i=0; i<kLogSize; ++i) {
    char buf[32];
    sprintf(buf, "test %d", i + kLogCount - kLogSize);
    expectSubstring(logs[i], buf);
  }
}
