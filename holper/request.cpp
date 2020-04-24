#include "request.h"
#include "context.h"
#include "info.h"
#include "consts.h"
#include "logger.h"
#include "string.h"
#include "exception.h"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

int Request::idCounter_;

std::string Response::serialize() {
  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
  value_.Accept(writer);
  return std::string(buf.GetString(), buf.GetSize());
}

Response& Response::set(std::string key, const std::string& value) {
  value_.AddMember(
      rapidjson::Value(key.c_str(), key.size(), alloc()),
      rapidjson::Value(value.c_str(), value.size(), alloc()),
      alloc());
  return *this;
}
Response& Response::set(std::string key, const char* value) {
  value_.AddMember(
      rapidjson::Value(key.c_str(), key.size(), alloc()),
      rapidjson::Value(value, strlen(value), alloc()),
      alloc());
  return *this;
}
Response& Response::set(std::string key, std::nullptr_t UNUSED(value)) {
  value_.AddMember(
      rapidjson::Value(key.c_str(), key.size(), alloc()),
      rapidjson::Value(rapidjson::kNullType),
      alloc());
  return *this;
}

Response& Response::set(std::string key, rapidjson::Value& value) {
  value_.AddMember(
      rapidjson::Value(key.c_str(), key.size(), alloc()),
      value.Move(),
      alloc());
  return *this;
}

rapidjson::Value& Parameters::getRaw(const std::string& name, bool* exists) {
  // ugh
  static rapidjson::Value dummy;
  auto it = rawParameters_.find(name);
  if (it == rawParameters_.end()) {
    if (exists) {
      *exists = false;
    }
    return dummy;
  }
  if (exists) {
    *exists = true;
  }
  return it->second;
}

void Request::sendResponse(int code) {
  profiler_.event("sendResponse called");
  response_.set("code", code);
  if (verbose_) {
    response_.set("profiler", profiler_.json(response_.alloc()).Move());
  }
  response_.set("id", id_);
  std::string msg = response_.serialize();
  profiler_.event("Write started");
  socket_->write(msg);
  profiler_.event("Write finished");
  context_->logger->info(
    "Response for request %d: %s%s%s\nRequest Stats: %s%s%s",
    id_,
    Consts::TerminalColors::YELLOW,
    msg.c_str(),
    Consts::TerminalColors::DEFAULT,
    Consts::TerminalColors::PURPLE,
    profiler_.str().c_str(),
    Consts::TerminalColors::DEFAULT
  );
  socket_.reset(nullptr);
}

void Request::setVerbose(bool verbose) {
  verbose_ = verbose;
}

Request::~Request() {
  context_->logger->info("Request %d destroyed", id_);
}

std::string ParamSpec::help() {
  if (paramSpecs_.empty()) {
    return "Takes no arguments";
  }
  std::stringstream ss;
  ss << "Arguments:\n";
  for (const auto& spec : paramSpecs_) {
    ss << "  " << spec.name;
    if (spec.type != "none") {
      ss << ":VALUE(" << spec.type << ")";
    }
    ss << ": " << spec.help << "\n";
  }
  for (const auto& [name, minmax] : keyRestrictions_) {
    int min = minmax.first, max = minmax.second;
    if (min == 1 && max == 1) {
      ss << "Must have one of:";
    } else if (min == 0 && max == 1) {
      ss << "Must have at most one of:";
    } else if (min == 1 && max == -1) {
      ss << "Must have at least one of:";
    } else if (max == -1) {
      ss << "Must have at least " << min << " of:";
    } else if (min == 0 && max == -1) {
      ss << "Must have zero or more of:";
    } else {
      ss << "Must have at least " << min << " and at most " << max << "of:";
    }
    for (const auto& spec : paramSpecs_) {
      if (spec.keys.find(name) != spec.keys.end()) {
        ss << " " << spec.name;
      }
    }
  }
  return ss.str();
}
