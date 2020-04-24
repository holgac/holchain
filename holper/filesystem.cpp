#include "filesystem.h"
#include "exception.h"
#include "string.h"
#include <stdarg.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void Filesystem::urandom(int len, void* data) {
  int fd = open("/dev/urandom", O_RDONLY);
  ssize_t readlen = ::read(fd, data, len);
  if(readlen < len) {
    if(readlen < 0) {
      THROW("Could not read /dev/urandom: %s", St::errorString().c_str());
    }
    THROW("Could not read enough data (%ld/%ld)", readlen, len);
  }
  close(fd);
}

std::string Filesystem::read(const std::string& path) {
  FILE* f = fopen(path.c_str(), "re");
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  std::string buf(size, '\0');
  fseek(f, 0, SEEK_SET);
  fread(&buf[0], 1, size, f);
  fclose(f);
  return buf;
}

int Filesystem::parse(const std::string& path, const char* format, ...) {
  FILE* f = fopen(path.c_str(), "re");
  va_list args;
  va_start(args, format);
  int res = vfscanf(f, format, args);
  va_end(args);
  fclose(f);
  return res;
}

int Filesystem::dump(const std::string& path, const char* format, ...) {
  FILE* f = fopen(path.c_str(), "we");
  va_list args;
  va_start(args, format);
  int res = vfprintf(f, format, args);
  va_end(args);
  fclose(f);
  return res;
}
