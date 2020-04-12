#pragma once
#include <string>

namespace StringUtils {
  std::string fmt(const char* fmt, ...);
  std::string errorString();
  std::string errorString(int err);
  template <typename T>
  bool to(const std::string&, T&);
}

namespace St = StringUtils;
