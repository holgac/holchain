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
  void write(const char* string, size_t len) {
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
  void _log(const char* string, size_t len) {
    for (auto &target: targets_) {
      target.get()->write(string, len);
    }
  }
  const std::map<Level, const char*> levelColors_ = {
    {DEBUG, Consts::TerminalColors::WHITE},
    {INFO, Consts::TerminalColors::WHITE},
    {WARN, Consts::TerminalColors::YELLOW},
    {MUSTFIX, Consts::TerminalColors::YELLOW},
    {ERROR, Consts::TerminalColors::RED},
    {FATAL, Consts::TerminalColors::RED},
    {INTERNAL, Consts::TerminalColors::PURPLE},
  };
public:
  void addTarget(LogTarget* target) {
    targets_.push_back(std::unique_ptr<LogTarget>(target));
  }
  // TODO: cpp file
  // TODO: something like stringstream
  // TODO: only log if verbosity allows
  // TODO: error() info() etc. methods and macros w/file+line
  void log(Level level, const char* format, ...) {
    const size_t BUFLEN = 8192;
    char buf[BUFLEN];
    va_list ap;
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    size_t size = strftime(buf, BUFLEN, "%Y-%m-%d %H:%M:%S", &tm);
    char threadname[32];
    pthread_getname_np(pthread_self(), threadname, 32);
    size += snprintf(buf + size, BUFLEN - size,
        " [%lld:%lld,%s]: %s", (long long)getpid(), (long long)gettid(),
        threadname, levelColors_.at(level));
    va_start(ap, format);
    int vsnlen = vsnprintf(buf + size, BUFLEN - size, format, ap);
    va_end(ap);
    int deflen = strlen(Consts::TerminalColors::DEFAULT);
    size += vsnlen;
    if (vsnlen < 0 || size + deflen + 1 >= BUFLEN) {
      if (level != INTERNAL) {
        log(INTERNAL, "Can't log \"%s\"", format);
      } else {
        std::string msg("Can't even log the format string");
        _log(msg.c_str(), msg.size());
      }
      return;
    }
    memcpy(buf + size, Consts::TerminalColors::DEFAULT, deflen);
    buf[size + deflen] = '\n';
    _log(buf, size + deflen + 1);
  }
};


