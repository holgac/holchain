#pragma once
#include <time.h>
#include <string>
#include <vector>
#include <sstream>

template <clock_t Clock>
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
  double operator-(const TimePointBase<Clock>& rhs) const {
    return value() - rhs.value();
  }
  TimePointBase<Clock> operator+(double rhs) const {
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
    return TimePointBase<Clock>(ts);
  }
  TimePointBase<Clock> operator-(double rhs) const {
    return *this + (-rhs);
  }
  bool operator<(const TimePointBase<Clock>& rhs) const {
    if (time_.tv_sec == rhs.time_.tv_sec) {
      return time_.tv_nsec < rhs.time_.tv_nsec;
    }
    return time_.tv_sec < rhs.time_.tv_sec;
  }
  std::string str() const;
};

typedef TimePointBase<CLOCK_MONOTONIC> TimePoint;
typedef TimePointBase<CLOCK_REALTIME> RealTimePoint;
