#pragma once
#include <unistd.h>
#include <memory>
#include <vector>
#include <map>
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
  // TODO: something like stringstream
  template <typename... Args>
  void debug(const char* fmt, Args... args) {
    _log(Logger::DEBUG, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void info(const char* fmt, Args... args) {
    _log(Logger::INFO, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void warn(const char* fmt, Args... args) {
    _log(Logger::WARN, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void mustfix(const char* fmt, Args... args) {
    _log(Logger::MUSTFIX, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void error(const char* fmt, Args... args) {
    _log(Logger::ERROR, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void fatal(const char* fmt, Args... args) {
    _log(Logger::FATAL, St::fmt(fmt, args...));
  }
  template <typename... Args>
  void logErrno(const char* fmt, Args... args) {
    _logErrno(St::fmt(fmt, args...));
  }
};
