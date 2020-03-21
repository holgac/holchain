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
#include <list>
#include <memory>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#define SUCC_OR_RET(lg, fn) if(0 != fn) { (lg).log(Logger::ERROR, "%s failed", #fn); return 1; }

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

struct Context {
public:
  Logger logger;
  Context()
  {
  }
};


class LockMutex {
private:
  pthread_mutex_t* mutex_;
public:
  LockMutex(pthread_mutex_t* mutex) : mutex_(mutex) {
    pthread_mutex_lock(mutex_);
  }
  ~LockMutex() {
    pthread_mutex_unlock(mutex_);
  }
};

class ThreadBase
{
public:
  virtual void start() = 0;
};

template <typename T>
class Thread : public ThreadBase
{
private:
  std::list<std::unique_ptr<T>> messages_;
  pthread_t thread_;
  pthread_mutex_t messagesMutex_;
  sem_t messagesSemaphore_;
  bool running_;
  std::string name_;
protected:
  Context* context_;
public:
  Thread(std::string name, Context* context) : name_(name), context_(context) {}
  void sendMessage(T* t) {
    {
      LockMutex lock(&messagesMutex_);
      messages_.push_back(std::unique_ptr<T>(t));
    }
    sem_post(&messagesSemaphore_);
  }
  virtual void handleMessage(std::unique_ptr<T> msg) = 0;
  void stop() {
    running_ = false;
  }
  virtual void start() {
    // TODO: error handling
    thread_ = pthread_self();
    pthread_setname_np(thread_, name_.c_str());
    context_->logger.log(Logger::INFO, "Thread %s starting", name_.c_str());
    pthread_mutex_init(&messagesMutex_, NULL);
    sem_init(&messagesSemaphore_, 0, 0);
    running_ = true;
    while(running_) {
      sem_wait(&messagesSemaphore_);
      std::unique_ptr<T> msg;
      {
        LockMutex lock(&messagesMutex_);
        msg = std::move(messages_.front());
        messages_.pop_front();
      }
      handleMessage(std::move(msg));
    }
  }
};

void* start_thread(void* thread_ptr) {
  printf("start_thread!\n");
  ThreadBase* thread = (ThreadBase*)thread_ptr;
  thread->start();
  return NULL;
}

struct ParserArgs {
  int fd;
  ParserArgs(int fileDescriptor) : fd(fileDescriptor) {}
};

class Parser : public Thread<ParserArgs>
{
public:
  Parser(std::string name, Context* context) : Thread(name, context) {}
  void handleMessage(std::unique_ptr<ParserArgs> msg) {
    int fd = msg->fd;
    char buf[1024];
    int res = recv(fd, buf, 1024, 0);
    context_->logger.log(Logger::INFO, "fd %d received %s (%d)", fd, buf, res);
    send(fd, "HELLO\n", 6, 0);
    close(fd);
  }
};

class Server
{
private:
  std::string path_;
  int socket_;
  // TODO: shared_ptr?
  Context context_;
  bool running_ = true;
  Parser* parser_;
  int initLogger() {
    if (isatty(STDOUT_FILENO)) {
      context_.logger.addTarget(new FDLogTarget(STDOUT_FILENO, false));
    }
    char logdir[1024];
    sprintf(logdir, "/run/user/%d/holperd.log", getuid());
    int logfd = open(logdir, O_APPEND | O_CREAT | O_DIRECT | O_DSYNC, S_IRUSR | S_IWUSR);
    if (!logfd) {
      return 1;
    }
    context_.logger.addTarget(new FDLogTarget(logfd, true));
    return 0;
  }
  int validateSocketPath() {
    struct stat statbuf;
    if (0 != stat(path_.c_str(), &statbuf)) {
      context_.logger.log(Logger::INFO,
          "%s does not exist, will create", path_.c_str());
    } else {
      if(0 != unlink(path_.c_str())) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        context_.logger.log(Logger::FATAL,
            "%s already exists, unlink failed with %s",
            path_.c_str(), errbuf);
        return 1;
      }
      context_.logger.log(Logger::INFO,
          "%s already exists, unlinked", path_.c_str());
    }
    return 0;
  }
  int createSocket() {
    socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket < 0) {
      char errbuf[1024];
      strerror_r(errno, errbuf, 1024);
      context_.logger.log(Logger::FATAL,
          "Socket creation failed: %s",
          path_.c_str(), errbuf);
      return 1;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path_.c_str(), sizeof(addr.sun_path)-1);
    if (0 != bind(socket_, (struct sockaddr*)&addr, sizeof(addr))) {
      char errbuf[1024];
      strerror_r(errno, errbuf, 1024);
      context_.logger.log(Logger::FATAL,
          "Socket bind failed: %s",
          errbuf);
      return 1;
    }
    return 0;
  }
public:
  Context* getContext() { return &context_; }
  Server(std::string path) : path_(path) {
  }
  int init() {
    SUCC_OR_RET(context_.logger, initLogger())
    SUCC_OR_RET(context_.logger, validateSocketPath())
    SUCC_OR_RET(context_.logger, createSocket())
    context_.logger.log(Logger::INFO, "renaming thread");
    SUCC_OR_RET(context_.logger, pthread_setname_np(pthread_self(), "MainServer"))
    context_.logger.log(Logger::INFO, "renamed thread");
    parser_ = new Parser("Parser", &context_);
    pthread_t tmp;
    context_.logger.log(Logger::INFO, "starting thread");
    pthread_create(&tmp, NULL, start_thread, (void*)parser_);
    context_.logger.log(Logger::INFO, "started thread");
    context_.logger.log(Logger::INFO, "Initialisation complete");
    return 0;
  }
  void start() {
    if(0 != listen(socket_, 10)) {
      context_.logger.log(Logger::FATAL, "Listen failed");
    }
    while (running_) {
      int fd = accept(socket_, NULL, NULL);
      if (fd < 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        context_.logger.log(Logger::FATAL,
            "Accept failed: %s", errbuf);
        continue;
      }
      context_.logger.log(Logger::INFO, "Got a new connection");
      parser_->sendMessage(new ParserArgs(fd));
    }
  }
  void setRunning(bool running) {
    running_ = running;
  }
};

int run_server(int argc, char** argv) {
  char socketpathraw[512];
  sprintf(socketpathraw, "/run/user/%d/holperd.sock", getuid());
  std::string socket_path(socketpathraw);
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
  Server server(vm["socket-path"].as<std::string>());
  SUCC_OR_RET(server.getContext()->logger, server.init())
  server.start();
  return 0;
}

int main(int argc, char** argv)
{
  return run_server(argc, argv);
}
