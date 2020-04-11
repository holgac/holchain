#include "exception.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

HException::HException(const char* file, const char* line, const char* fmt, ...)
  : file_(file), line_(line) {
  // TODO: use StringUtils
  va_list ap;
  va_start(ap, fmt);
  int vsnlen = vsnprintf(nullptr, 0, fmt, ap);
  va_end(ap);
  size_t filelen = strlen(file_);
  size_t linelen = strlen(line_);
  // message_ format: "<file>:<line>: <error message>"
  message_ = new char[filelen + 1 + linelen + 2 + vsnlen + 1];
  memcpy(message_, file_, filelen);
  message_[filelen] = ':';
  memcpy(message_+filelen+1, line_, linelen);
  message_[filelen+1+linelen] = ':';
  message_[filelen+1+linelen+1] = ' ';
  va_start(ap, fmt);
  vsnlen = vsnprintf(message_+filelen+1+linelen+2, vsnlen+1, fmt, ap);
  va_end(ap);
}
