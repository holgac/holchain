#pragma once

class Context;
class Command;

class MusicCommandGroup
{
public:
  static void registerCommands(Context* context, Command* command);
};
