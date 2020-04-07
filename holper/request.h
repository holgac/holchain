#pragma once
#include "holper.h"
#include "consts.h"
#include "logger.h"
#include "sdbus.h"
#include <variant>
#include <map>
#include <vector>
#include <string>
#include <unistd.h>

class Command;
class Request;

class Response {
  friend class Request;
  typedef std::variant<int, long long, double, bool, std::string> Value;
  std::map<std::string, Value> values_;
  void serializeValue(std::ostream& out, Value value) {
    auto ival = std::get_if<int>(&value);
    if (ival) {
      out << *ival;
      return;
    }
    auto lval = std::get_if<long long>(&value);
    if (lval) {
      out << *lval;
      return;
    }
    auto dval = std::get_if<double>(&value);
    if (dval) {
      out << *dval;
      return;
    }
    auto bval = std::get_if<bool>(&value);
    if (bval) {
      if (*bval) {
        out << "true";
      } else {
        out << "false";
      }
      return;
    }
    auto sval = std::get_if<std::string>(&value);
    if (sval) {
      out << "\"";
      for ( auto c : *sval ) {
        if (false);
        else if (c == '"') out << "\\\"";
        else if (c == '\\') out << "\\\\";
        else if (c == '/') out << "\\/";
        else if (c == '\b') out << "\\b";
        else if (c == '\f') out << "\\f";
        else if (c == '\n') out << "\\n";
        else if (c == '\r') out << "\\r";
        else if (c == '\t') out << "\\t";
        else out << c;
      }
      out << "\"";
      return;
    }
    THROW("This should not have been possible but here we are");
  }

public:
  Response& set(const std::string& key, Value value) {
    values_[key] = value;
    return *this;
  }
  std::string serialize() {
    std::stringstream ss;
    ss << "{";
    for( const auto& value : values_ ) {
      ss << "\"" << value.first << "\":";
      serializeValue(ss, value.second);
      ss << ",";
    }
    auto str = ss.str();
    str.back() = '}';
    return str;
  }
};

class Request {
  /*
   * current request lifecycle:
   * Server thread creates(only id populated)
   * Parser thread fills up raw args and then resolves (command+resolved)
   * Worker thread executes action and sends response
   */
  // TODO: not thread safe
  static int idCounter_;
  int socket_;
  int id_;
  bool rawArgsSet_ = false;
  std::vector<std::string> rawArgs_;
  bool argsSet_ = false;
  std::vector<std::string> args_;
  bool commandSet_ = false;
  bool resolved_ = false;
  std::shared_ptr<Command> command_;
  Response response_;
  std::shared_ptr<Context> context_;
  // TODO: add some profiling stuff here
public:
  explicit Request(std::shared_ptr<Context> context, int socket)
      : socket_(socket), id_(++idCounter_), context_(context) {
    if (socket_ < 0) {
      THROW("Error checks in socket creation failed?");
    }
  }
  ~Request() {
    if (socket_ >= 0) {
      context_->logger->log(Logger::ERROR, "Request %d never responded!", id_);
    }
  }
  int id() const {
    return id_;
  }
  int socket() const {
    return socket_;
  }
  void setRawArgs(std::vector<std::string>&& rawArgs) {
    // TODO: surround with #if DEBUG or something more c++
    if (rawArgsSet_) {
      THROW("Setting rawArgs twice in request %d!", id_);
    }
    rawArgs_ = rawArgs;
    rawArgsSet_ = true;
  }
  std::vector<std::string>& rawArgs() {
    if (!rawArgsSet_) {
      THROW("Getting rawArgs before setting in request %d!", id_);
    }
    return rawArgs_;
  }
  const std::vector<std::string>& rawArgs() const {
    if (!rawArgsSet_) {
      THROW("Getting rawArgs before setting in request %d!", id_);
    }
    return rawArgs_;
  }
  void setArgs(std::vector<std::string>&& args) {
    if (argsSet_) {
      THROW("Setting args twice in request %d!", id_);
    }
    args_ = args;
    argsSet_ = true;
  }
  std::vector<std::string>& args() {
    if (!argsSet_) {
      THROW("Getting args before setting in request %d!", id_);
    }
    return args_;
  }
  const std::vector<std::string>& args() const {
    if (!argsSet_) {
      THROW("Getting args before setting in request %d!", id_);
    }
    return args_;
  }
  void setCommand(const std::shared_ptr<Command>& command, bool resolved) {
    if (commandSet_) {
      THROW("Setting command twice in request %d!", id_);
    }
    command_ = command;
    commandSet_ = true;
    resolved_ = resolved;
  }
  std::shared_ptr<Command>& command() {
    if (!commandSet_) {
      THROW("Getting command before setting in request %d!", id_);
    }
    return command_;
  }
  const std::shared_ptr<Command>& command() const {
    if (!commandSet_) {
      THROW("Getting command before setting in request %d!", id_);
    }
    return command_;
  }
  bool resolved() const {
    return resolved_;
  }
  Response& response() {
    return response_;
  }
  const Response& response() const {
    return response_;
  }
  void respond() {
    if (socket_ < 0) {
      THROW("Request %d attempting to respond multiple times!", id_);
    }
    std::string msg = response_.serialize();
    context_->logger->log(Logger::INFO, "Response for request %d: %s%s%s", id_,
        Consts::TerminalColors::YELLOW,
        msg.c_str(),
        Consts::TerminalColors::DEFAULT
    );
    write(socket_, msg.c_str(), msg.size());
    close(socket_);
    socket_ = -1;
  }
};
