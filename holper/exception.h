#pragma once
#include <exception>
#include "holper.h"

#define CEXCEPTION(EType, ...) EType(__FILE__, TO_STRING(__LINE__), __VA_ARGS__)
#define EXCEPTION(...) HException(__FILE__, TO_STRING(__LINE__), __VA_ARGS__)
#define THROW(...) throw EXCEPTION(__VA_ARGS__)
#define CTHROW(...) throw CEXCEPTION(__VA_ARGS__)
#define UNREACHABLE THROW("Unreachable code")

// TODO: more exception types
class HException : public std::exception
{
  char* message_ = nullptr;
  const char* file_;
  const char* line_;
public:
  HException(const char* file, const char* line, const char* fmt, ...);
  HException(HException&& he) {
    message_ = he.message_;
    he.message_ = nullptr;
    file_ = he.file_;
    line_ = he.line_;
  }
  ~HException() {
    if (message_) {
      delete[] message_;
    }
  }
  const char* what() const noexcept {
    return message_;
  }
};


