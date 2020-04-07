#pragma once
#include "thread.h"
#include <vector>
#include <queue>
#include <list>
#include <string>
#include <memory>
#include "request.h"

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
  std::unique_ptr<Request> request;
  explicit WorkPoolArgs(std::unique_ptr<Request>&& req)
      : request(std::move(req)) {}
};

class WorkPool : public Thread<WorkPoolArgs>
{
public:
  class Work
  {
  public:
    std::unique_ptr<Request> request;
    explicit Work(std::unique_ptr<Request>&& req)
        : request(std::move(req)) {}
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
  std::unique_ptr<Request> getWork();
  void init();
};
