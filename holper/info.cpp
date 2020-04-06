#include "info.h"
#include "holper.h"
#include <chrono>
#include "command.h"
#include "consts.h"
#include "logger.h"

class StatsAction : public Action
{
public:
  StatsAction(std::shared_ptr<Context> context) : Action(context) {}
  std::string act(boost::program_options::variables_map UNUSED(vm)) const {
    std::stringstream stream;
    auto now = std::chrono::steady_clock::now();
    auto uptime_us = std::chrono::duration_cast<std::chrono::microseconds>(now - context_->startSteadyTime).count();
    double uptime = 0.000001 * uptime_us;
    char timebuf[64];
    strftime(timebuf, 64, "%Y-%m-%d %H:%M:%S", &context_->startTime);
    // TODO: tokenize when returning a map
    stream << "Holper " << HOLPER_VERSION << "\n"
      << "Up since " << timebuf << "\n"
      << "Uptime: " << uptime << "s\n"
    ;
    return stream.str();
  }
};

void InfoCommandGroup::registerCommands(std::shared_ptr<Context> context,
      std::shared_ptr<Command> command)
{
  command
    ->name("info")
    ->help("Internal info and stats for the daemon");
  command->addChild()
    ->name("stats")
    ->help("Internal stats")
    ->action(new StatsAction(context));
}
