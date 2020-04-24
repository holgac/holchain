#include "system.h"
#include "holper.h"
#include "command.h"
#include "sdbus.h"
#include "workpool.h"
#include <optional>
#include <string>

class Login1Action : public Action
{
private:
  const char* kLogin1Service = "org.freedesktop.login1";
  const char* kLogin1Object = "/org/freedesktop/login1";
  const char* kLogin1IFace = "org.freedesktop.login1.Manager";
  std::string method_;
public:
  Login1Action(Context* context, std::string method)
      : Action(context), method_(method) {}
  void spec(ParamSpec& UNUSED(spec)) const override {
    return;
  }
  rapidjson::Value actOn(Work* UNUSED(work)) const override {
    SDBus bus(kLogin1Service, kLogin1Object, kLogin1IFace, false);
    bus.call(method_.c_str(), "b", false);
    return rapidjson::Value("Success");
  }
};

void SystemCommandGroup::initializeCommand(Context* context,
    Command* command) {
  (*command)
    .setName("system")
    .setDescription("System control");
  (*command->addChild())
    .setName("suspend")
    .setDescription("Suspend the system")
    .makeAction<Login1Action>(context, "Suspend");
  (*command->addChild())
    .setName("hibernate")
    .setDescription("Hibernates the system")
    .makeAction<Login1Action>(context, "SuspendThenHibernate");
  (*command->addChild())
    .setName("reboot")
    .setDescription("Reboots the system")
    .makeAction<Login1Action>(context, "Reboot");
  (*command->addChild())
    .setName("poweroff").setName("shutdown")
    .setDescription("Shuts down the system")
    .makeAction<Login1Action>(context, "PowerOff");
}
