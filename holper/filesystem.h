#pragma once
#include <string>

namespace Filesystem {
  std::string read(const std::string& path);
  int parse(const std::string& path, const char* format, ...);
  int dump(const std::string& path, const char* format, ...);
}

namespace Fs = Filesystem;
