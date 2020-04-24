#include "command.h"
#include "holper.h"
#include "request.h"
#include "exception.h"
#include <set>

Command::Command(Command* parent) : parent_(parent) {}

void Command::validate() {
  std::set<std::string> names;
  for ( auto& child : children_ ) {
    for ( auto& name : child->names_ ) {
      auto it = names.find(name);
      if (it != names.end()) {
        THROW("Name %s reused in %s", name.c_str(), primaryName_.c_str());
      }
      names.insert(name);
    }
  }
  if (children_.empty() != (bool)action_) {
    THROW("Command %s has %s children %s action",
        primaryName_.c_str(),
        children_.empty() ? "neither" : "both",
        children_.empty() ? "nor" : "and"
      );
  }
  for ( auto& child : children_ ) {
    child->validate();
  }
}

Command& Command::setName(const std::string& name, bool primary) {
  names_.push_front(name);
  if (primary || primaryName_.empty()) {
      primaryName_ = name;
  }
  return *this;
}
Command& Command::setDescription(const std::string& description) {
  description_ = description;
  return *this;
}
Command& Command::setAction(std::unique_ptr<Action> action) {
  action_ = std::move(action);
  return *this;
}

Command* Command::addChild() {
  auto it = children_.insert(children_.end(), std::make_unique<Command>(this));
  return it->get();
}

Command* Command::getChild(std::string name) {
  for (auto& cmd : children_) {
    if (std::find(cmd->names_.begin(), cmd->names_.end(), name)
        != cmd->names_.end()) {
      return cmd.get();
    }
  }
  return nullptr;
}

const Action* Command::action() const {
  return action_.get();
}

std::string Command::name() const {
  return primaryName_;
}

std::string Command::help() const {
  std::stringstream ss;
  ss << primaryName_ << ": " << description_ << "\n";
  if (names_.size() > 1) {
    ss << "Aliases: ";
    bool first = true;
    for ( const auto& name : names_ ) {
      if (name == primaryName_) {
        continue;
      }
      if (first ) {
        ss << name;
        first = false;
        continue;
      }
      ss << ", " << name;
    }
    ss << "\n";
  }
  if (action_) {
    ParamSpec ps(Parameters({}, {}), false);
    action_->spec(ps);
    ss << ps.help();
    return ss.str();
  }
  ss << "Subcommands:\n";
  for ( const auto& child : children_ ) {
    ss << "  " << child->primaryName_ << ": " << child->description_ << "\n";
  }
  return ss.str();
}
