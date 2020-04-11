#pragma once

class Context;
class Command;

class InfoCommandGroup
{
public:
  static void registerCommands(Context* context, Command* command);
};
