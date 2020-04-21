#include "display.h"
#include "holper.h"
#include "command.h"
#include "context.h"
#include "logger.h"
#include "string.h"
#include "request.h"
#include "exception.h"
#include "workpool.h"
#include <fstream>
#include <algorithm>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/dpms.h>

class BrightnessChangeAction : public Action
{
  const std::string kBrightnessPath = "/sys/class/backlight/amdgpu_bl0";
  void setScreenPower(bool val) const {
    context_->logger->info("Turning display %s", val ? "on" : "off");
    Display *dpy;
    dpy = XOpenDisplay(NULL);
    int dummy;
    if (!DPMSQueryExtension(dpy, &dummy, &dummy)) {
      context_->logger->error("no DPMS extension");
      THROW("No DPMS extension - cannot turn display off");
    }
    CARD16 timeout;
		DPMSGetTimeouts(dpy, &timeout, &timeout, &timeout);
    if (val) {
      DPMSEnable(dpy);
      DPMSForceLevel(dpy, DPMSModeOn);
    } else {
			DPMSEnable(dpy);
			usleep(100000);
			DPMSForceLevel(dpy, DPMSModeOff);
    }
    XCloseDisplay(dpy);
  }
public:
  BrightnessChangeAction(Context* context) : Action(context) {}
protected:
  std::optional<std::string> failReason(Work* work) const override {
    auto& params = work->parameters();
    int opcnt = 0;
    bool exists;
    // TODO: validator
    if (!params.get<int>("incr", &exists) && exists) {
      return St::fmt("incr value not int: %s",
          params.get<std::string>("incr")->c_str());
    }
    opcnt += (int)exists;
    if (!params.get<int>("set", &exists) && exists) {
      return St::fmt("set value not int: %s",
          params.get<std::string>("set")->c_str());
    }
    opcnt += (int)exists;
    if (opcnt == 0) {
      return "No operation was specified";
    }
    if (opcnt > 1) {
      return "Got multiple operations";
    }
    return std::nullopt;
  }
  rapidjson::Value actOn(Work* work) const override {
    auto& params = work->parameters();
    // TODO: helpers for reading sys files
    int max_raw_brightness, raw_brightness;
    {
      std::ifstream max_brightness_file(kBrightnessPath + "/max_brightness");
      max_brightness_file >> max_raw_brightness;
    }
    {
      std::ifstream brightness_file(kBrightnessPath + "/brightness");
      brightness_file >> raw_brightness;
    }
    float brightness = raw_brightness * 1.0f / max_raw_brightness;
    float new_brightness = brightness;
    if (auto incr = params.get<int>("incr")) {
      new_brightness += *incr * 0.01f;
    } else if (auto set = params.get<int>("set")) {
      new_brightness = *set * 0.01f;
    }
    new_brightness = std::clamp(new_brightness, 0.0f, 1.0f);
    if (brightness < 0.001f && new_brightness > brightness) {
      setScreenPower(true);
    } else if (new_brightness < .001f && new_brightness < brightness) {
      new_brightness = 0.0f;
      setScreenPower(false);
    }
    {
      std::ofstream brightness_file(kBrightnessPath + "/brightness");
      raw_brightness = new_brightness * max_raw_brightness;
      brightness_file << raw_brightness;
    }
    rapidjson::Value val(rapidjson::kObjectType);
    val.AddMember("old_brightness",
        rapidjson::Value(brightness),
        work->allocator());
    val.AddMember("new_brightness",
        rapidjson::Value(new_brightness),
        work->allocator());
    return val;
  }

  std::string help() const override {
    return "  Arguments:\n"
      "    incr:[AMOUNT}: increase brightness by amount\n"
      "    set:[VALUE}: set brightness to value\n";
  }
};

void DisplayCommandGroup::initializeCommand(Context* context,
    Command* command) {
  (*command)
    .setName("display")
    .setDescription("Display control");
  (*command->addChild())
    .setName("brightness").setName("bri")
    .setDescription("Change brightness")
    .makeAction<BrightnessChangeAction>(context);
}
