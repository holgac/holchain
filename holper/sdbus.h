#pragma once
#include <systemd/sd-bus.h>
#include "consts.h"
#include "exception.h"

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
  SDBus(const char* service, const char* object, const char* iface, bool user)
      : service_(service), object_(object), iface_(iface) {
    int r;
    if (user) {
      r = sd_bus_open_user(&bus_);
    } else {
      r = sd_bus_open_system(&bus_);
    }
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
  template <typename... Args>
  void call(const char* method, const char* signature="", Args... args) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = nullptr;
    int r = sd_bus_call_method(bus_, service_, object_, iface_, method, &error,
        &m, signature, args...);
    if (r < 0) {
      auto e = CEXCEPTION(HSDBusException, error, "Failed to call %s: %s(%d)", method, error.message, -r);
      sd_bus_message_unref(m);
      throw e;
    }
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
  }
};

