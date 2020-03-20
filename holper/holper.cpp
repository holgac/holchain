#include <boost/program_options.hpp>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <cstdarg>
#include <cstdlib>
#include <vector>
#include <memory>
#include <ctime>

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

namespace Consts {
  namespace TerminalColors {
    const char* BLACK = "\033[90m";
    const char* RED = "\033[91m";
    const char* GREEN = "\033[92m";
    const char* YELLOW = "\033[93m";
    const char* BLUE = "\033[94m";
    const char* CYAN = "\033[95m";
    const char* PURPLE = "\033[96m";
    const char* WHITE = "\033[97m";
    const char* DEFAULT = "\033[0m";
  };
}

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
  };
public:
  void addTarget(LogTarget* target) {
    targets_.push_back(std::unique_ptr<LogTarget>(target));
  }

  void log(Level level, const char* format, ...) {
    const size_t BUFLEN = 2048;
    char buf[BUFLEN];
    va_list ap;
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    size_t size = strftime(buf, BUFLEN, "%Y-%m-%d %H:%M:%S", &tm);
    size += snprintf(buf + size, BUFLEN - size,
        " [%lld]: %s", (long long)getpid(), levelColors_.at(level));
    va_start(ap, format);
    int vsnlen = vsnprintf(buf + size, BUFLEN - size, format, ap);
    va_end(ap);
    int deflen = strlen(Consts::TerminalColors::DEFAULT);
    size += vsnlen;
    if (vsnlen < 0 || size + deflen + 1 >= BUFLEN) {
      _log("CAN'T LOG", 9);
      return;
    }
    memcpy(buf + size, Consts::TerminalColors::DEFAULT, deflen);
    buf[size + deflen] = '\n';
    _log(buf, size + deflen + 1);
  }
};
int run_server(int argc, char** argv) {
  char username[512];
  getlogin_r(username, 512);
  std::string socket_path = "/run/user/" + std::string(username) + "/holperd.sock";
  boost::program_options::options_description desc("Options");
  desc.add_options()
    ("help", "help help")
    ("socket-path,S",
      boost::program_options::value<std::string>(&socket_path)->default_value(socket_path))
  ;
  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::command_line_parser(argc, argv).options(desc).run(),
      vm);
  boost::program_options::notify(vm);
  std::cout << "socket-path: " << vm["socket-path"].as<std::string>() << std::endl;
  Logger logger;
  logger.addTarget(new FDLogTarget(STDOUT_FILENO, false));
  logger.log(Logger::INFO, "info hey %d", 5);
  logger.log(Logger::WARN, "warn hey %d", 1);
  logger.log(Logger::ERROR, "error hey %d", 2);
  return 0;
}

int main(int argc, char** argv)
{
  return run_server(argc, argv);
}
