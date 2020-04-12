#pragma once

class Context;
class Command;

class PulseCommandGroup
{
public:
  static void initializeCommand(Context* context, Command* command);
};
