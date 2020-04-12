#pragma once

class Context;
class Command;

class InfoCommandGroup
{
public:
  static void initializeCommand(Context* context, Command* command);
};
