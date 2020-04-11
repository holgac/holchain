#include "profiler.h"
std::string Profiler::str() {
  std::stringstream ss;
  TimePoint now;
  ss << "Started " << (now - start_).str() << " ago\n";
  ss << "Events: \n";
  for (const auto& event : events_ ) {
    ss << "  " << event.first << ": " << (event.second - start_).str() << "\n";
  }
  return ss.str();
}
rapidjson::Value Profiler::json(rapidjson::Document::AllocatorType& alloc) {
  rapidjson::Value val(rapidjson::kArrayType);
  for (const auto& event : events_ ) {
    rapidjson::Value elem(rapidjson::kArrayType);
    elem.PushBack(
      rapidjson::Value(event.first.c_str(), event.first.size(), alloc),
      alloc);
    elem.PushBack(
      rapidjson::Value((event.second - start_).value()),
      alloc);
    val.PushBack(elem, alloc);
  }
  return val;
}
