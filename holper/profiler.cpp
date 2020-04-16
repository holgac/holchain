#include "profiler.h"

std::string Profiler::str() {
  std::stringstream ss;
  TimePoint now;
  ss << "Started " << (now - start_).str() << " ago\n";
  ss << "Events: \n";
  for (const auto& event : events_ ) {
    ss << "  " << event.second << ": " << (event.first - start_).str() << "\n";
  }
  return ss.str();
}

rapidjson::Value Profiler::json(rapidjson::Document::AllocatorType& alloc) {
  rapidjson::Value val(rapidjson::kArrayType);
  for (const auto& event : events_ ) {
    rapidjson::Value elem(rapidjson::kArrayType);
    elem.PushBack(
      rapidjson::Value(event.second.c_str(), event.second.size(), alloc),
      alloc);
    elem.PushBack(
      rapidjson::Value((event.first - start_).value()),
      alloc);
    val.PushBack(elem, alloc);
  }
  return val;
}

Profiler& Profiler::operator+=(const Profiler& prof) {
  for (const auto& prof_event : prof.events_) {
    event(prof_event.second, prof_event.first);
  }
  return *this;
}
