#include "info.h"
#include "holper.h"
#include <iomanip>
#include "command.h"
#include "consts.h"
#include "logger.h"

class StatsAction : public Action
{
public:
  StatsAction(std::shared_ptr<Context> context) : Action(context) {}
  std::string act(boost::program_options::variables_map UNUSED(vm)) const {
    std::stringstream stream;
    // TODO: tokenize when returning a map
    stream << "Holper " << HOLPER_VERSION << "\n"
      << "Up since " << context_->startRealTime.str() << "\n"
      << "Uptime: " << context_->startTime.str() << "\n"
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
