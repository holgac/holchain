#pragma once
#include "holper.h"
#include "time.h"
#include <string>
#include <functional>
#include <memory>

class Context;

class UnixSocket {
private:
  std::string path_;
  int socket_;
  Context* context_;
  TimePoint ctime_;
  UnixSocket(UnixSocket& socket) = delete;
  void readRaw(u64 len, void* dest);
public:
  UnixSocket(Context* context, const std::string& path);
  UnixSocket(Context* context, int socket);
  UnixSocket(UnixSocket&& socket);
  ~UnixSocket();
  void connect();
  void serve(std::function<void(std::unique_ptr<UnixSocket>)> fn);
  void write(const std::string& data);
  std::string read();
  TimePoint ctime();
};
