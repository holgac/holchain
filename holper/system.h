#pragma once
#include <memory>

class Context;
class Command;

class SystemCommandGroup
{
public:
  static void initializeCommand(Context* context, Command* command);
};
