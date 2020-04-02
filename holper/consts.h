#pragma once
#include <chrono>
#include <ctime>
#include <memory>
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
  std::shared_ptr<Logger> logger;
  std::shared_ptr<Parser> parser;
  std::shared_ptr<Server> server;
  std::shared_ptr<WorkPool> workPool;
  std::shared_ptr<CommandManager> commandManager;
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

