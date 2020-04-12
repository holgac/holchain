#include "info.h"
#include "holper.h"
#include "command.h"
#include "consts.h"
#include "request.h"
#include "context.h"
#include "exception.h"
#include <rapidjson/document.h>

class StatsAction : public Action
{
protected:
  std::optional<std::string> failReason(
      const Parameters& params) const override {
    if (!params.map().empty()) {
      return "Stats action takes no parameter";
    }
    return std::nullopt;
  }
  rapidjson::Value actOn(const Parameters& UNUSED(params),
      rapidjson::Document::AllocatorType& alloc) const override {
    rapidjson::Value val(rapidjson::kObjectType);
    val.AddMember("version", HOLPER_VERSION, alloc);
    std::string st = context_->stats.startRealTime.str();
    val.AddMember("starttime",
        rapidjson::Value(st.c_str(), st.size(), alloc), alloc);
    st = context_->stats.startTime.str();
    val.AddMember("uptime",
        rapidjson::Value(st.c_str(), st.size(), alloc), alloc);
    return val;
  }
  std::string help() const override {
    return "";
  }
public:
  StatsAction(Context* context) : Action(context) {}
};

void InfoCommandGroup::initializeCommand(Context* context,
    Command* command)
{
  (*command)
    .setName("info")
    .setDescription("Internal info and stats for the daemon");
  (*command->addChild())
    .setName("stats")
    .setDescription("Internal stats")
    .makeAction<StatsAction>(context);
}
