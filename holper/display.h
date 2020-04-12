#pragma once

class Context;
class Command;

class DisplayCommandGroup
{
public:
  static void initializeCommand(Context* context, Command* command);
};
