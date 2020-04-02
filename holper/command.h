#pragma once
#include <memory>
#include <string>
#include <list>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
class Context;

class Action
{
protected:
  std::shared_ptr<Context> context_;
public:
  Action(std::shared_ptr<Context> context) : context_(context) {}
  // TODO: these should return a map
  virtual std::string act(boost::program_options::variables_map vm) const = 0;
  virtual boost::optional<boost::program_options::options_description>
      options() const {
    return boost::none;
  }
  virtual boost::optional<boost::program_options::positional_options_description>
      positionalOptions() const {
    return boost::none;
  }
};

class Command
{
  std::list<std::string> names_;
  std::string primaryName_;
  std::string help_;
  std::unique_ptr<Action> action_;
  std::list<std::shared_ptr<Command>> children_;
  boost::program_options::options_description options_;
  boost::program_options::positional_options_description positionalOptions_;
public:
  Command();
  void validate();
  Command* name(std::string name, bool primary=false);
  Command* help(std::string help);
  Command* action(Action* action);
  std::shared_ptr<Command> addChild();
  std::shared_ptr<Command> getChild(std::string name);
  bool hasAction();
  const Action* getAction();
  std::string helpMessage();
};
