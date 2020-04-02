#pragma once
#include <memory>

class Context;
class Command;

namespace InfoCommandGroup
{
  void registerCommands(std::shared_ptr<Context> context,
      std::shared_ptr<Command> command);
}
