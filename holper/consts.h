#pragma once

class Logger;
class Parser;
class Server;
class WorkPool;

// TODO: these are not consts
struct Context {
  std::unique_ptr<Logger> logger;
  std::unique_ptr<Parser> parser;
  std::unique_ptr<Server> server;
  std::unique_ptr<WorkPool> workPool;
  Context()
  {
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

