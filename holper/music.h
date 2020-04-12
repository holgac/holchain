#pragma once

class Context;
class Command;

class MusicCommandGroup
{
public:
  static void initializeCommand(Context* context, Command* command);
};
