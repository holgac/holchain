#pragma once
#include <time.h>
#include <string>
#include <vector>
#include <sstream>

template <clock_t Clock, class Derived>
class TimePointBase {
protected:
  struct timespec time_;
public:
  TimePointBase() {
    clock_gettime(Clock, &time_);
  }
  TimePointBase(struct timespec ts) {
    time_ = ts;
  }
  double value() const {
    return time_.tv_sec + 0.000000001 * time_.tv_nsec;
  }
  struct timespec timespec() {
    return time_;
  }
  double operator-(const Derived& rhs) const {
    return value() - rhs.value();
  }
  Derived operator+(double rhs) const {
    struct timespec ts;
    ts.tv_sec = time_.tv_sec + (long)rhs;
    ts.tv_nsec = time_.tv_nsec + (rhs - (long)rhs) * 1000000000.0;
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_nsec -= 1000000000;
      ts.tv_sec += 1;
    } else if (ts.tv_nsec < 0) {
      ts.tv_nsec += 1000000000;
      ts.tv_sec -= 1;
    }
    return Derived(ts);
  }
  Derived operator-(double rhs) const {
    return *this + (-rhs);
  }
  bool operator<(const Derived& rhs) const {
    if (time_.tv_sec == rhs.time_.tv_sec) {
      return time_.tv_nsec < rhs.time_.tv_nsec;
    }
    return time_.tv_sec < rhs.time_.tv_sec;
  }
};

class TimePoint : public TimePointBase<CLOCK_MONOTONIC, TimePoint> {
  const std::vector<std::pair<int, std::string>> kTimeUnits = {
    {7*24*60*60, "week"},
    {24*60*60, "day"},
    {60*60, "hour"},
    {60, "minute"},
    {1, "second"},
  };
public:
  std::string str() const {
    std::stringstream ss;
    double diff = TimePoint() - *this;
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
    ss << (int)((diff - (int)diff) * 1000.0) << "ms";
    return ss.str();
  }
};
class RealTimePoint : public TimePointBase<CLOCK_REALTIME, RealTimePoint> {
public:
  std::string str() const {
    char buf[32];
    struct tm tm;
    localtime_r(&time_.tv_sec, &tm);
    strftime(buf, 64, "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
  }
};
