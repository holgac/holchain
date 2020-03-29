#pragma once
#include <chrono>
#include <ctime>
#define HOLPER_VERSION_MAJOR 0
#define HOLPER_VERSION_MINOR 0
#define HOLPER_VERSION_REVISION 3
#define TO_STRING(s) TO_STRING2(s)
#define TO_STRING2(s) #s
#define HOLPER_VERSION "v" TO_STRING(HOLPER_VERSION_MAJOR) "." \
  TO_STRING(HOLPER_VERSION_MINOR) "." \
  TO_STRING(HOLPER_VERSION_REVISION)

class Logger;
class Parser;
class Server;
class WorkPool;
class CommandManager;

// TODO: these are not consts
struct Context {
  std::unique_ptr<Logger> logger;
  std::unique_ptr<Parser> parser;
  std::unique_ptr<Server> server;
  std::unique_ptr<WorkPool> workPool;
  std::unique_ptr<CommandManager> commandManager;
  // TODO: time abstraction
  struct tm startTime;
  std::chrono::steady_clock::time_point startSteadyTime;
  Context()
  {
    time_t t = time(NULL);
    localtime_r(&t, &startTime);
    startSteadyTime = std::chrono::steady_clock::now();
  }
};

namespace Consts {
  namespace TerminalColors {
    const char* BLACK = "\033[90m";
    const char* RED = "\033[91m";
    const char* GREEN = "\033[92m";
    const char* YELLOW = "\033[93m";
    const char* BLUE = "\033[94m";
    const char* CYAN = "\033[95m";
    const char* PURPLE = "\033[96m";
    const char* WHITE = "\033[97m";
    const char* DEFAULT = "\033[0m";
  };
}

