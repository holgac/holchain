#pragma once

class Context;
class Command;

class PulseCommandGroup
{
public:
  static void registerCommands(Context* context, Command* command);
};
