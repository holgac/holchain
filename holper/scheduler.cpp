#include "scheduler.h"
#include "holper.h"
#include "context.h"
#include "logger.h"
#include "thread.h"
#include "exception.h"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

class SchedulerWorkerThread : public ThreadBase
{
private:
  pthread_cond_t cond_;
  pthread_mutex_t mutex_;
  Scheduler* scheduler_;
public:
  SchedulerWorkerThread(Context* context, Scheduler* schedule)
      : ThreadBase("Scheduler", context), scheduler_(schedule) {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&cond_, &attr);
    pthread_mutex_init(&mutex_, NULL);
    pthread_condattr_destroy(&attr);
  }
  void wakeUp() {
    LockMutex lock(&mutex_);
    pthread_cond_signal(&cond_);
  }
  void run();
  void actionFinishedCallback(int requestId,
      std::shared_ptr<rapidjson::Document> UNUSED(doc),
      rapidjson::Value response, int code) {
    context_->logger()->info() << requestId << " finished successfully";
  }
};

struct ScheduledAction {
  // TODO: just use Work
  Command* command;
  Parameters parameters;
  int index;
  int requestId;
  ScheduledAction(Command* c, Parameters&& params,
      int idx, int reqId)
    : command(c), parameters(std::move(params)), index(idx), requestId(reqId) {}
};

void SchedulerWorkerThread::run() {
  while(1) {
    context_->logger->info("Checking in");
    LockMutex lock(&mutex_);
    auto lowest = scheduler_->lowest();
    if (!lowest) {
      context_->logger->info("No scheduled commands, sleeping");
      pthread_cond_wait(&cond_, &mutex_);
      continue;
    }
    TimePoint now;
    if (now > lowest) {
      auto action = scheduler_->pop();
      context_->logger->info("Running scheduled command %d from request %d",
          action->index, action->requestId);
      auto doc = std::make_shared<rapidjson::Document>();
      context_->workPool->sendMessage(std::make_unique<Work>(action->requestId,
            action->command, action->parameters, doc->GetAllocator(), nullptr,
            std::bind(&SchedulerWorkerThread::actionFinishedCallback, this,
              action.release(), doc, std::placeholders::_1, std::placeholders::_2));

      auto res = action->command()->action()->actOn(
        action.parameters,
        doc.GetAllocator());
    } else {
      context_->logger->info() << "Waiting for "
        << ((*lowest)-now).str().c_str();
      auto ts = lowest->timespec();
      pthread_cond_timedwait(&cond_, &mutex_, &ts);
    }
  }
}
/*
  ScheduledAction(Command* c, Parameters&& params,
      int idx, int reqId)
 */
class ScheduleAddAction : public Action
{
protected:
  std::optional<std::string> failReason(
      const Parameters& params) const override {
    if (!params.getRaw("command").IsArray()) {
      return "command is not a list";
    }
    if (!params.getRaw("parameters").IsObject()) {
      return "parameters is not a dict";
    }
    return std::nullopt;
  }
  rapidjson::Value actOn(const Parameters& params,
      rapidjson::Document::AllocatorType& alloc,
      int requestId) const override {
    std::vector<std::string> command_tokens;
    for ( auto& val : params.getRaw("command").GetArray() ) {
      command_tokens.push_back(std::string(val.GetString(), val.GetStringLength()));
    }
    std::map<std::string, std::string> parameters;
    std::map<std::string, rapidjson::Value> raw_parameters;
    for ( auto& val : params.getRaw("parameters").GetObject() ) {
      auto& name = val.name;
      auto& value = val.value;
      if (value.IsString()) {
        parameters.insert(std::make_pair<std::string, std::string>(
              std::move(std::string(name.GetString(), name.GetStringLength())),
              std::move(std::string(value.GetString(), value.GetStringLength()))));
      } else {
        raw_parameters.insert(std::make_pair<std::string, rapidjson::Value>(
          std::move(std::string(name.GetString(), name.GetStringLength())),
          std::move(value.Move())
        ));
      }
    }
    Parameters params(std::move(parameters), std::move(raw_parameters));
    TimePoint now;
    size_t id = context_->scheduler->add(now,
      context_->commandManager->resolveCommand(command_tokens),
      std::move(params), requestId);
    return rapidjson::Value(id);
  }
  std::string help() const override {
    return "";
  }
public:
  StatsAction(Context* context) : Action(context) {}
};

void SchedulerCommandGroup::initializeCommand(Context* context,
    Command* command)
{
  (*command)
    .setName("schedule").setName("sch").setName("delay")
    .setDescription("Scheduler a command for later execution");
  (*command->addChild())
    .setName("add").setName("insert").setName("i").setName("a")
    .setDescription("Scheduler a new job")
    .makeAction<ScheduleAddAction>(context);
}

