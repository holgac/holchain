#include "commandmanager.h"
#include "holper.h"
#include "command.h"
#include "consts.h"
#include "logger.h"
#include "info.h"
#include "pulse.h"
#include "music.h"
#include "display.h"
#include "request.h"

void CommandManager::initializeCommand(
    std::function<void(Context*,Command*)> initializer
) {
  Command* cmd = root_->addChild();
  initializer(context_, cmd);
  cmd->validate();
}

CommandManager::CommandManager(Context* context)
    : root_(new Command(nullptr)), context_(context) {
  root_->setName("holper")
    .setDescription("Root command");
}

Command* CommandManager::resolveCommand(
    const std::vector<std::string> command_tokens) {
  Command* node = root_.get();
  for ( const auto& token : command_tokens ) {
    Command* child = node->getChild(token);
    if (!child) {
      return node;
    }
    node = child;
  }
  return node;
}
