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

std::string TimeDelta::str() const {
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
