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
  int id_;
public:
  WorkPoolWorker(int id, Context* context, WorkPool* workPool);
  void run();
};

struct WorkPoolArgs
{
  std::unique_ptr<Request> request;
  explicit WorkPoolArgs(std::unique_ptr<Request> req)
      : request(std::move(req)) {}
};

class WorkPool : public Thread<WorkPoolArgs>
{
public:
  class Work
  {
  public:
    std::unique_ptr<Request> request;
    TimePoint ctime;
    explicit Work(std::unique_ptr<Request> req)
        : request(std::move(req)) {}
  };
private:
  pthread_mutex_t workMutex_;
  sem_t workSemaphore_;
  std::queue<std::unique_ptr<Work>> works_;
  std::list<std::unique_ptr<WorkPoolWorker>> workers_;
public:
  WorkPool(Context* context, size_t poolSize);
  void handleMessage(std::unique_ptr<WorkPoolArgs> msg) override;
  // Only called from worker threads
  std::unique_ptr<Request> getRequest();
};
