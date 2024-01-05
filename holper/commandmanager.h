#pragma once
#include <functional>
#include <memory>

class Context;
class Command;

template <class T>
concept CommandGroup = requires (T) {
  { T::initializeCommand((Context*)nullptr, (Command*)nullptr) } -> std::same_as<void>;
};

class CommandManager
{
  std::unique_ptr<Command> root_;
  Context* context_;
  void initializeCommand(std::function<void(Context*,Command*)> initializer);
public:
  CommandManager(Context* context);
  ~CommandManager() {}
  template <CommandGroup T>
  void registerCommandGroup() {
    initializeCommand(T::initializeCommand);
  }
  Command* resolveCommand(const std::vector<std::string> command_tokens);
  const Command* root() {
    return root_.get();
  }
};
