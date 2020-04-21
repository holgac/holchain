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

struct WorkResult
{
  std::unique_ptr<Work> work;
  rapidjson::Value result;
  int code;
  WorkResult(std::unique_ptr<Work>&& w, rapidjson::Value&& r, int c)
    : work(std::move(w)), result(std::move(r.Move())), code(c)
  {}
};

class Work
{
  int requestId_;
  const Command* command_;
  const Parameters parameters_;
  rapidjson::Document doc_;
  typedef std::function<void(std::unique_ptr<WorkResult>)> FinishFunction;
  FinishFunction finish_;
  Profiler profiler_;
  rapidjson::Document::AllocatorType* allocator_;
  friend class WorkPoolWorker;

public:
  int requestId() const {
    return requestId_;
  }
  rapidjson::Document::AllocatorType& allocator() {
    return *allocator_;
  }
  const Command* command() const {
    return command_;
  }
  const Parameters& parameters() const {
    return parameters_;
  }

  Profiler& profiler() {
    return profiler_;
  }

  Work(int id,
      const Command* cmd,
      Parameters&& params,
      rapidjson::Document::AllocatorType& alloc,
      FinishFunction finish
  ) : requestId_(id), command_(cmd), parameters_(std::move(params)),
      finish_(finish), allocator_(&alloc) {}
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
