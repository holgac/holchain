#include "command.h"
#include "holper.h"
#include "request.h"
#include <boost/program_options.hpp>

boost::program_options::variables_map Action::parse(const Request* req) const {
  auto opt = options();
  auto popt = positionalOptions();
  auto parser = boost::program_options::command_line_parser(req->args());
  if (opt) {
    parser.options(*opt);
    if (popt) {
      parser.positional(*popt);
    }
  } else {
    boost::program_options::options_description opts("");
    opts.add_options();
    parser.options(opts);
  }
  boost::program_options::variables_map vm;
  boost::program_options::store(parser.run(), vm);
  boost::program_options::notify(vm);
  return vm;
}
bool Action::canActOn(const Request* req) const {
  try {
    parse(req);
    return true;
  } catch (boost::program_options::unknown_option& e) {
    return false;
  }
}

std::string Action::actOn(Request* req) const {
  auto vm = parse(req);
  return act(vm);
}

Command::Command() {}

void Command::validate() {
  // TODO: check for duplicate names
  // TODO: check if no children and action
  // TODO: check if !opt && popt
}

Command* Command::name(std::string name, bool primary) {
  names_.push_front(name);
  if (primary || primaryName_.empty()) {
      primaryName_ = name;
  }
  return this;
}
Command* Command::help(std::string help) {
  help_ = help;
  return this;
}
Command* Command::action(Action* action) {
  action_.reset(action);
  return this;
}

std::shared_ptr<Command> Command::addChild() {
  auto c = std::make_shared<Command>(Command());
  children_.push_back(c);
  return c;
}

std::shared_ptr<Command> Command::getChild(std::string name) {
  for (auto& cmd : children_) {
    if (std::find(cmd->names_.begin(), cmd->names_.end(), name) != cmd->names_.end()) {
      return cmd;
    }
  }
  return std::shared_ptr<Command>();
}

bool Command::hasAction() {
  return (bool)action_;
}

const Action* Command::getAction() const {
  return action_.get();
}

std::string Command::helpMessage() const {
  std::stringstream ss;
  if (action_) {
    auto opts = action_->options();
    if (opts) {
      ss << *opts;
    } else {
      ss << help_;
    }
    // TODO dup code
    if (names_.size() > 1) {
      ss << "\nAliases:";
      for (auto& name : names_) {
        ss << " " << name;
      }
      ss << "\n";
    }
    return ss.str();
  }
  ss << primaryName_ << ":" << help_ <<  "\n";
  if (names_.size() > 1) {
    ss << "Aliases:";
    for (auto& name : names_) {
      ss << " " << name;
    }
    ss << "\n";
  }
  ss << "Subcommands:\n";
  for (auto& child : children_) {
    ss << "\t" << child->primaryName_ << ": " << child->help_ << "\n";
  }
  return ss.str();
}
