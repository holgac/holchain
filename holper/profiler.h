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
  std::vector<std::pair<std::string, TimePoint>> events_;
public:
  Profiler() {}
  Profiler(TimePoint start) : start_(start) {}
  void event(const std::string& name) {
    events_.push_back(std::pair<std::string, TimePoint>(name, TimePoint()));
  }
  std::string str();
  rapidjson::Value json(rapidjson::Document::AllocatorType& alloc);
};
