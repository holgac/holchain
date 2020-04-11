#pragma once
#include <exception>
#include <string>
#include "holper.h"
#include "string.h"

#define CEXCEPTION(EType, ...) EType(__FILE__, __LINE__, __VA_ARGS__)
#define EXCEPTION(...) HException(__FILE__, __LINE__, __VA_ARGS__)
#define THROW(...) throw EXCEPTION(__VA_ARGS__)
#define CTHROW(...) throw CEXCEPTION(__VA_ARGS__)
#define UNREACHABLE THROW("Unreachable code")

// TODO: more exception types
class HException : public std::exception
{
  std::string message_;
  const char* file_;
  int line_;
public:
  template <typename... Args>
  HException(const char* file, int line, const char* fmt, Args... args)
    : message_(St::fmt((std::string("%s:%d: ") + fmt).c_str(),
          file, line, args...)),
      file_(file), line_(line) {
  }
  HException(HException&& he)
    : message_(std::move(he.message_)), file_(he.file_), line_(he.line_) {
  }
  ~HException() {}
  const char* what() const noexcept override {
    return message_.c_str();
  }
};
