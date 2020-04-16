#pragma once
#include <sstream>
#include <vector>
#include <utility>
#include <string>
#include <map>
#include "time.h"
#include <rapidjson/document.h>

class Profiler
{
  TimePoint start_;
  std::multimap<TimePoint, std::string> events_;
public:
  Profiler() {}
  Profiler(const TimePoint& start) : start_(start) {}
  void event(const std::string& name, const TimePoint tp = TimePoint()) {
    events_.insert(std::make_pair(tp, name));
  }
  std::string str();
  Profiler& operator+=(const Profiler& prof);
  rapidjson::Value json(rapidjson::Document::AllocatorType& alloc);
};
