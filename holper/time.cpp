#include "time.h"

namespace {
  const std::vector<std::pair<int, std::string>> kTimeUnits = {
    {7*24*60*60, "week"},
    {24*60*60, "day"},
    {60*60, "hour"},
    {60, "minute"},
    {1, "second"},
  };
  std::string secondsToString(double val) {
    std::stringstream ss;
    if (val < 0.001) {
      ss <<  val*1000000 << "ns";
    } else if (val < 1.0) {
      ss <<  val*1000 << "ms";
    } else {
      ss << val << "s";
    }
    return ss.str();
  }
}

std::string TimeDelta::str() {
  return secondsToString(value_);
}

template<>
std::string TimePoint::str() const {
  std::stringstream ss;
  double diff = (TimePoint() - *this).value();
  int secs = (int)diff;
  for (const auto& timeUnit : kTimeUnits) {
    if (secs >= timeUnit.first) {
      ss << (secs / timeUnit.first) << " " << timeUnit.second;
      if (secs >= 2 * timeUnit.first) {
        ss << "s";
      }
      ss << " ";
      secs %= timeUnit.first;
    }
  }
  ss << secondsToString((diff - (int)diff));
  return ss.str();
}
template <>
std::string RealTimePoint::str() const {
  char buf[32];
  struct tm tm;
  localtime_r(&time_.tv_sec, &tm);
  strftime(buf, 64, "%Y-%m-%d %H:%M:%S", &tm);
  return std::string(buf);
}
/*
class TimerThread : public ThreadBase
{
private:
  pthread_cond_t cond_;
  pthread_mutex_t mutex_;
public:
  TimerThread(std::shared_ptr<Context> context) : ThreadBase("Timer", context) {
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
};

class Timer {
  const int kMaxTimers = 20;
  std::multimap<TimePoint, std::function<std::string()>> actionQueue_;
  std::vector<boost::optional<std::multimap<TimePoint, std::function<std::string()>>::iterator>> actionIndex_;
  pthread_mutex_t mutex_;
  std::shared_ptr<Context> context_;
public:
  Timer(std::shared_ptr<Context> context) : context_(context) {
    actionIndex_.resize(kMaxTimers, boost::none);
    pthread_mutex_init(&mutex_, NULL);
  }
  int add(const TimePoint& time, std::function<std::string()> fn) {
    LockMutex lock(&mutex_);
    for(int i=0; i<kMaxTimers; ++i) {
      if (!actionIndex_[i]) {
        auto it = actionQueue_.insert(std::make_pair(time, fn));
        actionIndex_[i] = boost::make_optional(it);
        context_->timerThread->wakeUp();
        return i;
      }
    }
    THROW("Already have %d timers!", kMaxTimers);
  }
  boost::optional<TimePoint> lowest() {
    LockMutex lock(&mutex_);
    if (actionQueue_.empty()) {
      return boost::none;
    }
    return actionQueue_.begin()->first;
  }
  std::function<std::string()> pop() {
    LockMutex lock(&mutex_);
    auto it = actionQueue_.begin();
    for (int i=0; i<kMaxTimers; ++i) {
      if(actionIndex_[i] == it) {
        auto fn = it->second;
        actionIndex_[i] = boost::none;
        actionQueue_.erase(it);
        return fn;
      }
    }
    THROW("Corrupt index!");
  }
};

void TimerThread::run() {
  while(1) {
    context_->logger->log(Logger::INFO, "Checking in");
    LockMutex lock(&mutex_);
    auto lowest = context_->timer->lowest();
    if (!lowest) {
      context_->logger->log(Logger::INFO, "No scheduled commands, sleeping");
      pthread_cond_wait(&cond_, &mutex_);
      continue;
    }
    TimePoint now;
    if (now > lowest) {
      context_->logger->log(Logger::INFO, "Running scheduled command");
      auto fn = context_->timer->pop();
      fn();
      continue;
    }
    {
      context_->logger->log(Logger::INFO, "Waiting for %3.2lfs", (*lowest)-now);
      auto ts = lowest->timespec();
      pthread_cond_timedwait(&cond_, &mutex_, &ts);
    }
  }
}
*/
