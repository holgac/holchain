#pragma once
#include <systemd/sd-bus.h>
#define CEXCEPTION(EType, ...) EType(__FILE__, TO_STRING(__LINE__), __VA_ARGS__)
#define EXCEPTION(...) HException(__FILE__, TO_STRING(__LINE__), __VA_ARGS__)
#define THROW(...) throw EXCEPTION(__VA_ARGS__)
#define CTHROW(...) throw CEXCEPTION(__VA_ARGS__)

// TODO: more exception types
// TODO: not the right place
class HException : public std::exception
{
  char* message_ = nullptr;
  const char* file_;
  const char* line_;
public:
  HException(const char* file, const char* line, const char* fmt, ...)
    : file_(file), line_(line) {
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

class HSDBusException : public HException
{
  sd_bus_error error_ = SD_BUS_ERROR_NULL;
public:
  template<typename... Args>
  HSDBusException(const char* file, const char* line, sd_bus_error error, Args... args)
    : HException(file, line, args...), error_(error) {
  }
  HSDBusException(HSDBusException&& he) : HException(std::move(he)) {
    sd_bus_error_move(&error_, &he.error_);
  }
  ~HSDBusException() {
    sd_bus_error_free(&error_);
  }
  const char* errorName() {
    return error_.name;
  }
};

class SDBus
{
  sd_bus* bus_ = nullptr;
  const char* service_;
  const char* object_;
  const char* iface_;
public:
  SDBus(const char* service, const char* object, const char* iface)
      : service_(service), object_(object), iface_(iface) {
    int r;
    r = sd_bus_open_user(&bus_);
    if (r < 0) {
      char errbuf[1024];
      strerror_r(-r, errbuf, 1024);
      sd_bus_unref(bus_);
      THROW("Failed to connect to bus: %s", errbuf);
    }
  }
  ~SDBus() {
    if(bus_) {
      sd_bus_unref(bus_);
    }
  }
  void call(const char* method) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = nullptr;
    int r = sd_bus_call_method(bus_, service_, object_, iface_, method, &error, &m, "");
    if (r < 0) {
      auto e = CEXCEPTION(HSDBusException, error, "Failed to call %s: %s(%d)", method, error.message, -r);
      sd_bus_message_unref(m);
      throw e;
    }
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
  }
};

