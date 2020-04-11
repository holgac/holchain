#pragma once
#include <memory>

class Context;
class Command;

class SystemCommandGroup
{
public:
  static void registerCommands(Context* context, Command* command);
};
