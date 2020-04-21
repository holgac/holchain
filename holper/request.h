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
  std::map<std::string,rapidjson::Value> rawParameters_;
  void set(std::map<std::string,std::string>&& parameters,
      std::map<std::string, rapidjson::Value>&& rawParameters) {
    parameters_ = std::move(parameters);
    rawParameters_ = std::move(rawParameters);
  }
public:
  Parameters() {}
  Parameters(Parameters&& p)
    : parameters_(std::move(p.parameters_)),
      rawParameters_(std::move(p.rawParameters_)) {}

  Parameters(std::map<std::string,std::string>&& parameters,
        std::map<std::string, rapidjson::Value>&& rawParameters)
    : parameters_(std::move(parameters)),
      rawParameters_(std::move(rawParameters)) {}

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
  rapidjson::Value& getRaw(const std::string& name, bool* exists = nullptr);
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
  bool verbose_ = false;
  void setVerbose(bool verbose);
public:
  Request(Context* context, std::unique_ptr<UnixSocket> socket)
      : id_(++idCounter_), profiler_(socket->ctime()),
        socket_(std::move(socket)), context_(context) {
  }
  ~Request();
  int id() const {
    return id_;
  }
  Response& response() {
    return response_;
  }
  UnixSocket* socket() {
    return socket_.get();
  }
  Profiler& profiler() {
    return profiler_;
  }

  bool verbose() {
    return verbose_;
  }

  void sendResponse(int code);
};
