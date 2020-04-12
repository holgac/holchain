#include <gtest/gtest.h>
#include "logger.h"

class BufferingLogTarget : public LogTarget
{
private:
  std::vector<std::string> logs_;
public:
  const std::vector<std::string>& getLogs() {
    return logs_;
  }
  ~BufferingLogTarget(){}
  void write(const char* str, size_t size) override {
    logs_.push_back(std::string(str, size));
  }
};

class LoggerTest : public ::testing::Test {
protected:
  LoggerTest() : target_(new BufferingLogTarget) {
    logger_.addTarget(std::unique_ptr<LogTarget>(target_));
  }
  ~LoggerTest() override {}
  void SetUp() override {}
  void TearDown() override {
    // not deleting target_ since it's owned by logger
  }

  BufferingLogTarget* target_;
  Logger logger_;
};

TEST_F(LoggerTest, LogTargetWorks) {
  logger_.info("test");
  EXPECT_EQ(target_->getLogs().size(), 1);
}

TEST_F(LoggerTest, LogFormattingWorks) {
  const int kLogCount = 10;
  for (int i=0; i<kLogCount; ++i) {
    logger_.info("test %d", i);
  }
  EXPECT_EQ(target_->getLogs().size(), kLogCount);
  for (int i=0; i<kLogCount; ++i) {
    char buf[32];
    sprintf(buf, "test %d", i);
    EXPECT_NE(target_->getLogs()[i].find(buf), std::string::npos);
  }
}
