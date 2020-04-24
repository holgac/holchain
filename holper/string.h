#pragma once
#include <string>
#include <vector>
#include "exception.h"

namespace StringUtils {
  std::string fmt(const char* fmt, ...);
  std::string errorString();
  std::string errorString(int err);
  template <typename T>
  bool to(const std::string&, T&);
  template <typename T>
  std::vector<T> split(const std::string& str, char delim) {
    std::vector<T> vec;
    size_t left = 0;
    T t;
    while(true) {
      size_t right = str.find(delim, left);
      if (right == std::string::npos) {
        if (!to<T>(std::string(str.c_str()+left), t)) {
          THROW("Cannot parse %s", std::string(str.c_str()+left));
        }
        vec.push_back(t);
        return vec;
      }
      if (!to(std::string(str.c_str()+left, right - left), t)) {
        THROW("Cannot parse %s",
            std::string(str.c_str()+left, right - left).c_str());
      }
      left = right + 1;
      vec.push_back(t);
    }
  }
  template <typename T>
  std::string to();
}

namespace St = StringUtils;
