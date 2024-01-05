#pragma once

class Context;
class Command;

class Scheduler {
  const size_t kMaxQueueSize = 1000;
  typedef std::pair<TimePoint, std::unique_ptr<SchduledAction>> ActionPair;
  typedef std::multimap<TimePoint, std::unique_ptr<SchduledAction>> ActionMap;
  ActionMap actionQueue_;
  std::map<size_t, ActionMap::iterator> actionIndex_;
  pthread_mutex_t mutex_;
  Context* context_;
  std::unique_ptr<SchedulerWorkerThread> schedulerThread_;

public:
  Scheduler(Context* context) : context_(context) {
    pthread_mutex_init(&mutex_, NULL);
    schedulerThread_.reset(new SchedulerWorkerThread(context_, this));
    schedulerThread_->start();
  }

  size_t add(const TimePoint& time, Command* command,
      Parameters&& parameters, int requestId) {
    size_t new_idx = 0;
    LockMutex lock(&mutex_);
    for (new_idx = 0; new_idx < kMaxQueueSize; ++new_idx) {
      if (actionIndex_.find(new_idx) == actionIndex_.end()) {
        break;
      }
    }
    if (new_idx = kMaxQueueSize) {
      THROW("Already have %d timers!", kMaxQueueSize);
    }
    auto it = actionQueue_.insert(ActionPair(TimePoint(),
          std::make_unique<ScheduledAction>(
            command, std::move(parameters), new_idx, requestId)));
    actionIndex_.insert(std::make_pair(new_idx, it));
    schedulerThread_->wakeUp();
    return new_idx;
  }

  boost::optional<TimePoint> lowest() {
    LockMutex lock(&mutex_);
    if (actionQueue_.empty()) {
      return boost::none;
    }
    return actionQueue_.begin()->first;
  }
  std::unique_ptr<ScheduledAction> pop() {
    LockMutex lock(&mutex_);
    auto it = actionQueue_.begin();
    std::unique_ptr<ScheduledAction> action = std::move(it->second);
    actionQueue_.erase(it);
    actionIndex_.erase(actionIndex.find(action.index));
    return action;
  }
};

class SchedulerCommandGroup
{
public:
  static void initializeCommand(Context* context, Command* command);
};
