#pragma once
#include "thread.h"
#include <vector>
#include <queue>
#include <list>
#include <string>
#include <memory>
#include "request.h"

class WorkPool;
struct Work;

class WorkPoolWorker : public ThreadBase
{
private:
  WorkPool* workPool_;
  int id_;
  std::pair<rapidjson::Value, int> runSingle(Work* args);
public:
  WorkPoolWorker(int id, Context* context, WorkPool* workPool);
  void run();
};

struct Work
{
  int requestId;
  const Command* command;
  const Parameters* parameters;
  rapidjson::Document::AllocatorType* allocator;
  std::function<void(rapidjson::Value val, int code)> finish;
  Profiler* profiler;

  void profile(const std::string& event) {
    if (!profiler) {
      return;
    }
    profiler->event(event);
  }

  Work(Request* request,
      std::function<void(rapidjson::Value val, int code)> onFinish
  ) : requestId(request->id()), command(request->command()),
      parameters(&request->parameters()),
      allocator(&request->response().alloc()),
      finish(onFinish), profiler(&request->profiler()) {}

  Work(int id,
      const Command* cmd,
      const Parameters& params,
      rapidjson::Document::AllocatorType& alloc,
      std::function<void(rapidjson::Value val, int code)> onFinish,
      Profiler* prof
  ) : requestId(id), command(cmd), parameters(&params), allocator(&alloc),
      finish(onFinish), profiler(prof) {}
};

class WorkPool : public Thread<Work>
{
public:
  class WorkInternal
  {
  public:
    std::unique_ptr<Work> work;
    TimePoint ctime;
    explicit WorkInternal(std::unique_ptr<Work> workPtr)
        : work(std::move(workPtr)) {}
  };
private:
  pthread_mutex_t workMutex_;
  sem_t workSemaphore_;
  std::queue<std::unique_ptr<WorkInternal>> works_;
  std::list<std::unique_ptr<WorkPoolWorker>> workers_;
public:
  WorkPool(Context* context, size_t poolSize);
  void handleMessage(std::unique_ptr<Work> msg) override;
  // Only called from worker threads
  std::unique_ptr<Work> getWork();
};
