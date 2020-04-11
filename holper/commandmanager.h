#pragma once
#include <functional>
#include <memory>

class Context;
class Command;

class CommandManager
{
  std::unique_ptr<Command> root_;
  Context* context_;
public:
  CommandManager(Context* context);
  ~CommandManager() {}
  void registerCommandGroup(std::function<void(Context*,Command*)> factory);
  Command* resolveCommand(const std::vector<std::string> command_tokens);
  const Command* root() {
    return root_.get();
  }
};
