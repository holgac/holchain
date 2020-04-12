#include "logger.h"
#include "string.h"

LogLine::LogLine(std::function<void(const std::string&)> log) : log_(log) {
}

LogLine::~LogLine() {
  log_(ss.str());
}

void Logger::_logErrno(const std::string& log) {
  _log(ERROR, log + ": " + StringUtils::errorString());
}
void Logger::_logToTargets(const char* string, size_t len) {
  for (auto &target: targets_) {
    target.get()->write(string, len);
  }
}
void Logger::_log(Level level, const std::string& message) {
  if ((int)level < (int)verbosity_) {
    return;
  }
  std::stringstream ss;
  char thread_name[32];
  pthread_getname_np(pthread_self(), thread_name, 32);
  ss << RealTimePoint().str()
    << " [" << getpid() << ":" << gettid() << ":" << thread_name << "]: "
    << levelColors_.at(level) << message << Consts::TerminalColors::DEFAULT << "\n";
  std::string str = ss.str();
  _logToTargets(str.c_str(), str.size());
}

LogLine Logger::debug() {
  return LogLine(std::bind(&Logger::_log, this, DEBUG, std::placeholders::_1));
}

LogLine Logger::info() {
  return LogLine(std::bind(&Logger::_log, this, INFO, std::placeholders::_1));
}

LogLine Logger::warn() {
  return LogLine(std::bind(&Logger::_log, this, WARN, std::placeholders::_1));
}

LogLine Logger::mustfix() {
  return LogLine(std::bind(&Logger::_log, this, MUSTFIX, std::placeholders::_1));
}

LogLine Logger::error() {
  return LogLine(std::bind(&Logger::_log, this, ERROR, std::placeholders::_1));
}

LogLine Logger::fatal() {
  return LogLine(std::bind(&Logger::_log, this, FATAL, std::placeholders::_1));
}

LogLine Logger::logErrno() {
  return LogLine(std::bind(&Logger::_logErrno, this, std::placeholders::_1));
}
