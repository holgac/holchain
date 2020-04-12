#include "string.h"
#include "exception.h"
#include <stdarg.h>
#include <cstring>
#include <errno.h>

template <>
bool StringUtils::to(const std::string& str, int& val) {
  char tmp;
  return sscanf(str.c_str(), "%d%c", &val, &tmp) == 1;
}

template <>
bool StringUtils::to(const std::string& str, std::string& val) {
  val = str;
  return true;
}

template <>
bool StringUtils::to(const std::string& str, nullptr_t& UNUSED(val)) {
  return str.empty();
}

std::string StringUtils::fmt(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);
    char* buf = new char[len + 1];
    va_start(ap, fmt);
    vsnprintf(buf, len + 1, fmt, ap);
    va_end(ap);
    std::string str(buf, len);
    delete[] buf;
    return str;
}

std::string StringUtils::errorString() {
  return errorString(errno);
}

std::string StringUtils::errorString(int err) {
  char errbuf[1024];
  return std::string(strerror_r(err, errbuf, 1024));
}
