#pragma once
#include "time.h"
#include <memory>

class Logger;
class Server;
class WorkPool;
class CommandManager;
class Resolver;
class Responder;

struct Stats {
  TimePoint startTime;
  RealTimePoint startRealTime;
};

struct Context {
  std::shared_ptr<Logger> logger;
  std::shared_ptr<Resolver> resolver;
  std::shared_ptr<Responder> responder;
  std::shared_ptr<Server> server;
  std::shared_ptr<WorkPool> workPool;
  std::shared_ptr<CommandManager> commandManager;
  Stats stats;
  Context() {}
};
