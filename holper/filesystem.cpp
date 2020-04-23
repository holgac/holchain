#include "filesystem.h"
 #include <stdarg.h>
 #include <cstdio>

std::string Filesystem::read(const std::string& path) {
  FILE* f = fopen(path.c_str(), "r");
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  std::string buf(size, '\0');
  fseek(f, 0, SEEK_SET);
  fread(&buf[0], 1, size, f);
  fclose(f);
  return buf;
}

int Filesystem::parse(const std::string& path, const char* format, ...) {
  FILE* f = fopen(path.c_str(), "r");
  va_list args;
  va_start(args, format);
  int res = vfscanf(f, format, args);
  va_end(args);
  fclose(f);
  return res;
}

int Filesystem::dump(const std::string& path, const char* format, ...) {
  FILE* f = fopen(path.c_str(), "w");
  va_list args;
  va_start(args, format);
  int res = vfprintf(f, format, args);
  va_end(args);
  fclose(f);
  return res;
}
