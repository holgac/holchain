#pragma once
#include "command.h"

class CommandManager
{
  std::unique_ptr<Command> root_;
  std::shared_ptr<Context> context_;
public:
  CommandManager(std::shared_ptr<Context> context) : context_(context) {}
  void init();
  void resolveRequest(Request* req);
  const Command* rootCommand() {
    return root_.get();
  }
};


