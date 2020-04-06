#include "commandmanager.h"
#include "holper.h"
#include "consts.h"
#include "logger.h"
#include "info.h"
#include "pulse.h"
#include "music.h"
#include "display.h"
#include <boost/program_options.hpp>
void CommandManager::init() {
  root_.reset(new Command());
  // TODO: move these away, register command registering method instead
  InfoCommandGroup::registerCommands(context_, root_->addChild());
  PulseCommandGroup::registerCommands(context_, root_->addChild());
  MusicCommandGroup::registerCommands(context_, root_->addChild());
  DisplayCommandGroup::registerCommands(context_, root_->addChild());
}

std::string CommandManager::runCommand(std::vector<std::string> args) {
  boost::program_options::positional_options_description root_popt;
  root_popt.add("command", -1);
  boost::program_options::options_description root_opt;
  root_opt.add_options()
    ("schedule", boost::program_options::value<int>(),
      "Schedule the command instead of running immediately")
    ("command", boost::program_options::value<std::vector<std::string>>(), "The actual command")
  ;
  if (args.empty() || args[0] == "--help" || args[0] == "-h") {
    std::stringstream ss;
    ss << root_->helpMessage() << std::endl << root_opt;
    return ss.str();
  }
  boost::program_options::variables_map root_vm;
  auto root_parser = boost::program_options::command_line_parser(args);
  root_parser.options(root_opt);
  root_parser.positional(root_popt);
  try {
    boost::program_options::store(root_parser.run(), root_vm);
  } catch (boost::program_options::unknown_option& e) {
    context_->logger->log(Logger::ERROR, "Invalid command: %s\n", e.what());
    // TODO: dup code
    std::stringstream ss;
    ss << root_->helpMessage() << std::endl << root_opt;
    return ss.str();
  }
  boost::program_options::notify(root_vm);
  args = root_vm["command"].as<std::vector<std::string>>();
  auto node = root_->getChild(args[0]);
  if (!node) {
    context_->logger->log(Logger::ERROR, "Command %s not found", args[0].c_str());
    return "Can't find " + args[0];
  }
  size_t argidx = 1;

  for (;argidx<args.size(); ++argidx) {
    auto child = node->getChild(args[argidx]);
    if (!child) {
      break;
    }
    node = child;
  }
  bool print_help = false;
  // TODO: add --help to action's options instead of hard coding like this
  print_help |= argidx == args.size() - 1 &&
      (args[argidx] == "--help" || args[argidx] == "-h");
  print_help |= !node->hasAction();
  if (print_help) {
    return node->helpMessage();
  }
  boost::program_options::variables_map vm;
  auto opt = node->getAction()->options();
  auto popt = node->getAction()->positionalOptions();
  std::vector<std::string> rest(args.begin() + argidx, args.end());
  auto parser = boost::program_options::command_line_parser(rest);
  if (opt) {
    parser.options(*opt);
  } else {
    boost::program_options::options_description opts("");
    opts.add_options();
    parser.options(opts);
  }
  if (popt) {
    parser.positional(*popt);
  }
  try {
    boost::program_options::store(parser.run(), vm);
  } catch (boost::program_options::unknown_option& e) {
    context_->logger->log(Logger::ERROR, "Invalid command: %s\n", e.what());
    return node->helpMessage();
  }
  boost::program_options::notify(vm);
  if(!root_vm.count("schedule")) {
    return node->getAction()->act(vm);
  }
  return "schedule not yet implemented";
}
