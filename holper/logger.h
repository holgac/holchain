#pragma once
#include <unistd.h>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <pthread.h>
#include <stdarg.h>
#include "consts.h"
#include "string.h"

class LogTarget
{
public:
  virtual ~LogTarget(){}
  virtual void write(const char*, size_t) = 0;
};

class FDLogTarget : public LogTarget
{
private:
  int fd_;
  bool close_;
public:
  FDLogTarget(int fd, bool close) : fd_(fd), close_(close) {}
  virtual ~FDLogTarget() {
    if (close_) {
        close(fd_);
    }
  }
  void write(const char* string, size_t len) override {
    ::write(fd_, string, len);
  }
};

class InMemoryLogTarget : public LogTarget
{
private:
  std::vector<std::string> logs_;
  size_t index_ = 0;
  size_t logCount_ = 0;
public:
  std::vector<std::string> getLogs() {
    std::vector<std::string> vec;
    vec.reserve(logs_.size());
    if (logCount_ > index_) {
      for (size_t i = index_; i < logs_.size(); ++i) {
        vec.push_back(logs_[i]);
      }
    }
    for (size_t i = 0; i < index_; ++i) {
      vec.push_back(logs_[i]);
    }
    return vec;
  }

  InMemoryLogTarget(size_t size) : logs_(size) {
  }

  ~InMemoryLogTarget(){}

  void write(const char* str, size_t size) override {
    logs_[index_] = std::string(str, size);
    index_ = (index_ + 1) % logs_.size();
    logCount_ += 1;
  }
};

class Logger;

class LogLine {
  std::stringstream ss;
  std::function<void(const std::string&)> log_;
public:
  LogLine(std::function<void(const std::string&)>);
  ~LogLine();
  template <typename T>
  LogLine& operator<<(const T& val) {
    ss << val;
    return *this;
  }
};

class Logger
{
public:
  enum Level {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    MUSTFIX = 3,
    ERROR = 4,
    FATAL = 5,
    INTERNAL = 6,
  };
private:
  std::vector<std::unique_ptr<LogTarget>> targets_;
  void _logToTargets(const char* string, size_t len);
  void _logErrno(const std::string& log);
  const std::map<Level, const char*> levelColors_ = {
    {DEBUG, Consts::TerminalColors::WHITE},
    {INFO, Consts::TerminalColors::WHITE},
    {WARN, Consts::TerminalColors::YELLOW},
    {MUSTFIX, Consts::TerminalColors::YELLOW},
    {ERROR, Consts::TerminalColors::RED},
    {FATAL, Consts::TerminalColors::RED},
    {INTERNAL, Consts::TerminalColors::PURPLE},
  };
  void _log(Level level, const std::string& message);
  Level verbosity_;
public:
  Logger(Level verbosity = DEBUG) : verbosity_(verbosity) {}
  void addTarget(std::unique_ptr<LogTarget> target) {
    targets_.push_back(std::move(target));
  }
  LogLine debug();
  LogLine info();
  LogLine warn();
  LogLine mustfix();
  LogLine error();
  LogLine fatal();
  LogLine logErrno();

  template <typename... Args>
  void debug(const char* fmt, Args... args) {
    _log(DEBUG, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void info(const char* fmt, Args... args) {
    _log(INFO, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void warn(const char* fmt, Args... args) {
    _log(WARN, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void mustfix(const char* fmt, Args... args) {
    _log(MUSTFIX, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void error(const char* fmt, Args... args) {
    _log(ERROR, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void fatal(const char* fmt, Args... args) {
    _log(FATAL, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void logErrno(const char* fmt, Args... args) {
    _logErrno(St::fmt(fmt, args...));
  }
};
