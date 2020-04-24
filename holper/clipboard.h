#pragma once

class Context;
class Command;

class ClipboardCommandGroup
{
public:
  static void initializeCommand(Context* context, Command* command);
};
