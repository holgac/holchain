#pragma once
#include <ctime>
#include <memory>
#include "time.h"

class Logger;
class Parser;
class Server;
class WorkPool;
class CommandManager;

// TODO: these are not consts
struct Context {
  std::shared_ptr<Logger> logger;
  std::shared_ptr<Parser> parser;
  std::shared_ptr<Server> server;
  std::shared_ptr<WorkPool> workPool;
  std::shared_ptr<CommandManager> commandManager;
  // TODO: where should these live in?
  TimePoint startTime;
  RealTimePoint startRealTime;
  Context() {}
};

namespace Consts {
  namespace TerminalColors {
    extern const char* BLACK;
    extern const char* RED;
    extern const char* GREEN;
    extern const char* YELLOW;
    extern const char* BLUE;
    extern const char* CYAN;
    extern const char* PURPLE;
    extern const char* WHITE;
    extern const char* DEFAULT;
  };
}

