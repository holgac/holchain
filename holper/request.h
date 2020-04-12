#pragma once
#include "holper.h"
#include "socket.h"
#include "profiler.h"
#include "string.h"
#include <rapidjson/document.h>
#include <rapidjson/allocators.h>
#include <memory>
#include <vector>
#include <string>
#include <map>

class Command;
class Request;
class Resolver;

class Response {
private:
  friend class Request;
  rapidjson::Value value_;
  rapidjson::Document doc_;
  Response() : value_(rapidjson::kObjectType) {
  }
public:
  rapidjson::Document::AllocatorType& alloc() {
    return doc_.GetAllocator();
  }
  template <typename T>
  Response& set(std::string key, T value) {
    value_.AddMember(
        rapidjson::Value(key.c_str(), key.size(), alloc()),
        rapidjson::Value(value),
        alloc());
    return *this;
  }
  Response& set(std::string key, const std::string& value);
  Response& set(std::string key, const char* value);
  Response& set(std::string key, std::nullptr_t value);
  Response& set(std::string key, rapidjson::Value& value);
  std::string serialize();
};


class Parameters {
  friend class Request;
  std::map<std::string,std::string> parameters_;
  void set(std::map<std::string,std::string>&& parameters) {
    parameters_ = std::move(parameters);
  }
public:
  Parameters() {}
  const std::map<std::string,std::string>& map() const {
    return parameters_;
  }
  template <typename T>
  std::optional<T> get(const std::string& name, bool* exists = nullptr) const {
    auto it = parameters_.find(name);
    if (it == parameters_.end()) {
      if (exists) {
        *exists = false;
      }
      return std::nullopt;
    }
    if (exists) {
      *exists = true;
    }
    T val;
    if(St::to<T>(it->second, val)) {
      return val;
    }
    return std::nullopt;
  }
};

class Request {
  friend class Resolver;
  /*
   * current request lifecycle:
   * Server thread creates(only id populated)
   * Parser thread fills up raw args and then resolves (command+resolved)
   * Worker thread executes action and sends response
   */
  // TODO: not thread safe
  static int idCounter_;
  int id_;
  Profiler profiler_;
  std::unique_ptr<UnixSocket> socket_;
  Response response_;
  Context* context_;
  Command* command_ = nullptr;
  bool verbose_ = false;
  std::vector<std::string> commandTokens_;
  Parameters parameters_;
  void setVerbose(bool verbose);
  void setCommandTokens(std::vector<std::string>&& command_tokens);
  void setParameters(std::map<std::string,std::string>&& parameters);
  void setCommand(Command* command);
public:
  Request(Context* context, std::unique_ptr<UnixSocket> socket)
      : id_(++idCounter_), profiler_(socket->ctime()),
        socket_(std::move(socket)), context_(context) {
  }
  ~Request() {
  }
  int id() const {
    return id_;
  }
  Response& response() {
    return response_;
  }
  UnixSocket* socket() {
    return socket_.get();
  }
  Command* command();

  void sendResponse(int code);

  Profiler& profiler() {
    return profiler_;
  }
  const Parameters& parameters() {
    return parameters_;
  }
  bool verbose() {
    return verbose_;
  }
};
