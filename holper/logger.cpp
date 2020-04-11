#include "logger.h"
#include "string.h"

void Logger::_logErrno(const std::string& log) {
  _log(Logger::ERROR, log + ": " + StringUtils::errorString());
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
