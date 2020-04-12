#pragma once
#include "request.h"
#include <memory>
#include <string>
#include <list>
#include <map>
#include <optional>
#include <rapidjson/document.h>

class Context;
class Request;
class Response;
class Parameters;

class Action
{
protected:
  Context* context_;
public:
  Action(Context* context) : context_(context) {}
  virtual std::optional<std::string> failReason(const Parameters&) const = 0;
  virtual rapidjson::Value actOn(const Parameters&,
      rapidjson::Document::AllocatorType&) const = 0;
  virtual std::string help() const = 0;
};

class Command final
{
  std::list<std::string> names_;
  std::string primaryName_;
  std::string description_;
  std::unique_ptr<Action> action_;
  std::list<std::unique_ptr<Command>> children_;
  Command* parent_;
public:
  Command(Command* parent);
  void validate();
  Command& setName(const std::string& name, bool primary=false);
  Command& setDescription(const std::string& description);
  template <typename T, typename... Args>
  Command& makeAction(Args... args) {
    return setAction(std::make_unique<T>(args...));
  }
  Command& setAction(std::unique_ptr<Action> action);
  Command* addChild();
  Command* getChild(std::string name);
  const Action* action() const;
  std::string help() const;
  std::string name() const;
};
