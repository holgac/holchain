#pragma once
#include "thread.h"
#include <vector>
#include <queue>
#include <list>
#include <string>
#include <memory>

class WorkPool;

class WorkPoolWorker : public ThreadBase
{
private:
  WorkPool* workPool_;
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
  void handleMessage(std::unique_ptr<WorkPoolArgs> msg);
  // Only called from worker threads
  std::unique_ptr<Work> getWork();
  void init();
};
