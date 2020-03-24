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
#include <queue>
#include <memory>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#define SUCC_OR_RET(fn) SUCC_OR_RET_WITH_LOGGER(context_->logger, fn)
#define SUCC_OR_RET_WITH_LOGGER(lg, fn) if(0 != (fn)) { (lg)->log(Logger::ERROR, "%s failed", #fn); return 1; }
#include "consts.h"
#include "logger.h"
#include "thread.h"

class WorkPool;

class WorkPoolWorker : public ThreadBase
{
private:
  WorkPool *workPool_;
  bool running_;
public:
  WorkPoolWorker(std::string name, std::shared_ptr<Context> context, WorkPool* workPool)
      : ThreadBase(name, context), workPool_(workPool) {}
  void run();
};

struct WorkPoolArgs
{
  int socketFd;
  std::vector<std::string> args;
  WorkPoolArgs(int fd, std::vector<std::string> arguments)
      : socketFd(fd), args(arguments) {}
};

class WorkPool : public Thread<WorkPoolArgs>
{
public:
  class Work
  {
  public:
    // TODO: id
    int socketFd;
    std::vector<std::string> args;
    Work(){}
    Work(int fd, std::vector<std::string> arguments)
        : socketFd(fd), args(arguments) {}
  };
private:
  pthread_mutex_t workMutex_;
  sem_t workSemaphore_;
  std::queue<std::unique_ptr<Work>> works_;
  std::list<std::unique_ptr<WorkPoolWorker>> workers_;
  size_t poolSize_;
public:
  WorkPool(std::shared_ptr<Context> context, size_t poolSize)
      : Thread("WorkPool", context), poolSize_(poolSize) {}
  void handleMessage(std::unique_ptr<WorkPoolArgs> msg) {
    std::unique_ptr<Work> work(new Work(msg->socketFd, msg->args));
    {
      LockMutex lock(&workMutex_);
      works_.push(std::move(work));
    }
    context_->logger->log(Logger::INFO, "Received work of fd %d", msg->socketFd);
    sem_post(&workSemaphore_);
  }
  // Only called from worker threads
  std::unique_ptr<Work> getWork() {
    sem_wait(&workSemaphore_);
    std::unique_ptr<Work> work;
    {
      LockMutex lock(&workMutex_);
      work = std::move(works_.front());
      works_.pop();
    }
    return work;
  }
  void init() {
    for (size_t i=0; i<poolSize_; ++i) {
      char thread_name[32];
      sprintf(thread_name, "Worker%u", (unsigned)i);
      WorkPoolWorker* worker = new WorkPoolWorker(thread_name, context_, this);
      worker->start();
      workers_.push_back(std::unique_ptr<WorkPoolWorker>(worker));
    }
  }
};

void WorkPoolWorker::run() {
  running_ = true;
  while (running_) {
    auto work = workPool_->getWork();
    context_->logger->log(Logger::INFO, "Will process work of fd %d", work->socketFd);
    std::stringstream out;
    for(auto& item: work->args) {
      out << item << " and then ";
    }
    auto outstr = out.str();
    context_->logger->log(Logger::INFO, "Sending response: %s", outstr.c_str());
    write(work->socketFd, outstr.c_str(), outstr.size());
    close(work->socketFd);
  }
}

struct ParserArgs {
  int fd;
  ParserArgs(int fileDescriptor) : fd(fileDescriptor) {}
};

class Parser : public Thread<ParserArgs>
{
public:
  Parser(std::shared_ptr<Context> context) : Thread("Parser", context) {}
  void handleMessage(std::unique_ptr<ParserArgs> msg) {
    int fd = msg->fd;
    std::stringstream request;
    while (true) {
      char buf[1024];
      int res = recv(fd, buf, 1024, 0);
      request << std::string(buf, res);
      if (res < 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        context_->logger->log(Logger::ERROR,
            "recv failed: %s", errbuf);
        close(fd);
        return;
      }
      if (res == 0) {
        break;
      }
    }
    context_->logger->log(Logger::INFO, "fd %d received %s (%d)", fd, request.str().c_str(), (int)request.tellp());
    boost::property_tree::ptree tree;
    try {
      boost::property_tree::read_json(request, tree);
    } catch (boost::property_tree::json_parser::json_parser_error& _) {
      context_->logger->log(Logger::ERROR, "Json parsing failed");
      close(fd);
      return;
    }
    std::vector<std::string> args;
    for (auto& val: tree.get_child("")) {
      args.push_back(val.second.get_value<std::string>());
    }
    context_->workPool->sendMessage(new WorkPoolArgs(fd, args));
    context_->logger->log(Logger::INFO, "Sent to work pool");
  }
};

class Server
{
private:
  std::string path_;
  int socket_;
  std::shared_ptr<Context> context_;
  bool running_ = true;
  int initLogger() {
    if (isatty(STDOUT_FILENO)) {
      context_->logger->addTarget(new FDLogTarget(STDOUT_FILENO, false));
    }
    char logdir[1024];
    sprintf(logdir, "/run/user/%d/holperd.log", getuid());
    int logfd = open(logdir, O_APPEND | O_CREAT | O_DIRECT | O_DSYNC, S_IRUSR | S_IWUSR);
    if (!logfd) {
      return 1;
    }
    context_->logger->addTarget(new FDLogTarget(logfd, true));
    return 0;
  }
  int validateSocketPath() {
    struct stat statbuf;
    if (0 != stat(path_.c_str(), &statbuf)) {
      context_->logger->log(Logger::INFO,
          "%s does not exist, will create", path_.c_str());
    } else {
      if(0 != unlink(path_.c_str())) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        context_->logger->log(Logger::FATAL,
            "%s already exists, unlink failed with %s",
            path_.c_str(), errbuf);
        return 1;
      }
      context_->logger->log(Logger::INFO,
          "%s already exists, unlinked", path_.c_str());
    }
    return 0;
  }
  int createSocket() {
    socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket < 0) {
      char errbuf[1024];
      strerror_r(errno, errbuf, 1024);
      context_->logger->log(Logger::FATAL,
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
      context_->logger->log(Logger::FATAL,
          "Socket bind failed: %s",
          errbuf);
      return 1;
    }
    return 0;
  }
  int initContext() {
    context_.reset(new Context());
    context_->logger.reset(new Logger());
    context_->parser.reset(new Parser(context_));
    context_->parser->start();
    context_->workPool.reset(new WorkPool(context_, 4));
    context_->workPool->start();
    return 0;
  }
public:
  Server(std::string path) : path_(path) {
  }
  int init() {
    SUCC_OR_RET(initContext())
    SUCC_OR_RET(initLogger())
    SUCC_OR_RET(pthread_setname_np(pthread_self(), "Server"))
    SUCC_OR_RET(validateSocketPath())
    SUCC_OR_RET(createSocket())
    context_->logger->log(Logger::INFO, "Initialisation complete");
    return 0;
  }
  void start() {
    if(0 != listen(socket_, 10)) {
      context_->logger->log(Logger::FATAL, "Listen failed");
    }
    while (running_) {
      int fd = accept(socket_, NULL, NULL);
      if (fd < 0) {
        char errbuf[1024];
        strerror_r(errno, errbuf, 1024);
        context_->logger->log(Logger::FATAL,
            "Accept failed: %s", errbuf);
        continue;
      }
      context_->logger->log(Logger::INFO, "Got a new connection");
      context_->parser->sendMessage(new ParserArgs(fd));
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
  // TODO
  if(server.init() != 0) {
    return 1;
  }
  server.start();
  return 0;
}

int main(int argc, char** argv)
{
  return run_server(argc, argv);
}
