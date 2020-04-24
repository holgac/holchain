#pragma once
#include "holper.h"
#include "socket.h"
#include "profiler.h"
#include "string.h"
#include "exception.h"
#include <rapidjson/document.h>
#include <rapidjson/allocators.h>
#include <memory>
#include <vector>
#include <set>
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

class ParamSpec {
private:
  const Parameters* parameters_;
  std::map<std::string, int> counters_;
  std::set<std::string> parametersAccountedFor_;
  std::string failReason_;
  bool validate_;
  void addFailReason(const std::string& st) {
    failReason_ += st + ", ";
  }
  struct SingleParamSpec {
    std::string name;
    std::string type;
    std::string help;
    std::set<std::string> keys;
  };
  std::vector<SingleParamSpec> paramSpecs_;
  std::map<std::string, std::pair<int, int>> keyRestrictions_;
public:
  std::string help();
  ParamSpec(const Parameters& parameters, bool validate)
    : parameters_(&parameters), validate_(validate) {
  }
  template <typename T>
  ParamSpec& param(const std::string& name, const std::string& key,
      const std::string& help) {
    return param<T>(name, std::set<std::string>{key}, help);
  }
  template <typename T>
  ParamSpec& param(const std::string& name,
      const std::set<std::string>& keys, const std::string& help) {
    if (parametersAccountedFor_.find(name) != parametersAccountedFor_.end()) {
      THROW("Parameter %s specified multiple times!", name.c_str());
    }
    parametersAccountedFor_.insert(name);
      paramSpecs_.push_back(SingleParamSpec{name, St::to<T>(), help, keys});
    if (!validate_) {
      return *this;
    }
    bool exists;
    if (!parameters_->get<T>(name, &exists) && exists) {
      addFailReason(St::fmt("parameter %s is not %s: \"%s\"", name.c_str(),
          St::to<T>(), parameters_->get<std::string>(name)->c_str()));
    } else if (exists) {
      for (const auto& key : keys) {
        auto it = counters_.find(key);
        if (it == counters_.end()) {
          counters_.insert(std::make_pair(key, 1));
        } else {
          it->second++;
        }
      }
    }
    return *this;
  }
  ParamSpec& key(const std::string& key, int min, int max) {
    if (min < 0 || (max != -1 && max < min)) {
      THROW("Invalid key specification: %s (min:%d, max:%d)",
          key.c_str(), min, max);
    }
    keyRestrictions_[key] = std::make_pair(min, max);
    if (!validate_) {
      return *this;
    }
    auto it = counters_.find(key);
    int val = it == counters_.end() ? 0 : it->second;
    if (val < min) {
      addFailReason(St::fmt("has %d %s (min:%d)", val, key.c_str(), min));
    }
    if (val > max && max != -1) {
      addFailReason(St::fmt("has %d %s (max:%d)", val, key.c_str(), max));
    }
    return *this;
  }
  std::optional<std::string> failReason() {
    for (const auto& [key, val] : parameters_->map()) {
      if (parametersAccountedFor_.find(key) == parametersAccountedFor_.end()) {
        addFailReason(St::fmt("was not expecting param %s with value \"%s\"",
            key.c_str(), val.c_str()));
      }
    }
    if (failReason_.empty()) {
      return std::nullopt;
    } else {
      failReason_.pop_back();
      failReason_.pop_back();
      return failReason_;
    }
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
