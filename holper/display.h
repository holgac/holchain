#pragma once

class Context;
class Command;

class DisplayCommandGroup
{
public:
  static void registerCommands(Context* context, Command* command);
};
