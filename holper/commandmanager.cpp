#include "commandmanager.h"
#include "holper.h"
#include "consts.h"
#include "logger.h"
#include "info.h"
#include "pulse.h"
#include "music.h"
#include "display.h"
#include "request.h"
#include <boost/program_options.hpp>
void CommandManager::init() {
  root_.reset(new Command());
  // TODO: move these away, register command registering method instead
  InfoCommandGroup::registerCommands(context_, root_->addChild());
  PulseCommandGroup::registerCommands(context_, root_->addChild());
  MusicCommandGroup::registerCommands(context_, root_->addChild());
  DisplayCommandGroup::registerCommands(context_, root_->addChild());
}

void CommandManager::resolveRequest(Request* req) {
  const auto& raw_args = req->rawArgs();
  if (raw_args.empty()) {
    req->setCommand(std::shared_ptr<Command>(nullptr), false);
    return;
  }
  auto node = root_->getChild(raw_args[0]);
  if (!node) {
    req->setCommand(std::shared_ptr<Command>(nullptr), false);
    return;
  }
  size_t i = 1;
  bool resolved = true;
  for (; i < raw_args.size(); ++i) {
    auto child = node->getChild(raw_args[i]);
    if (!child) {
      resolved = node->hasAction();
      break;
    }
    node = child;
  }
  resolved = resolved && node->getAction() != nullptr;
  req->setCommand(node, resolved);
  req->setArgs(std::move(std::vector<std::string>(raw_args.begin()+i, raw_args.end())));
}
