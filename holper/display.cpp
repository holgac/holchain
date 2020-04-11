#include "display.h"
#include "holper.h"
#include "command.h"
#include "context.h"
#include "logger.h"
#include "string.h"
#include "request.h"
#include "exception.h"
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
  std::optional<std::string> failReason(Request* req) const {
    const auto& params = req->parameters();
    auto it = params.end();
    int opcnt = 0;
    int tmp;
    if ((it = params.find("incr")) != params.end()) {
      opcnt++;
      if (!St::to<int>(it->second, &tmp)) {
        return St::fmt("incr value not int: %s", it->second.c_str());
      }
    }
    if ((it = params.find("set")) != params.end()) {
      opcnt++;
      if (!St::to<int>(it->second, &tmp)) {
        return St::fmt("set value not int: %s", it->second.c_str());
      }
    }
    if (opcnt == 0) {
      return "No operation was specified";
    }
    if (opcnt > 1) {
      return "Got multiple operations";
    }
    return std::nullopt;
  }
  rapidjson::Value actOn(Request* req) const {
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
    const auto& params = req->parameters();
    auto it = params.end();
    if ((it = params.find("incr")) != params.end()) {
      int incr;
      St::to<int>(it->second, &incr);
      new_brightness += incr * 0.01f;
    }
    if ((it = params.find("set")) != params.end()) {
      int incr;
      St::to<int>(it->second, &incr);
      new_brightness = incr * 0.01f;
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
    auto& alloc = req->response().alloc();
    val.AddMember("old_brightness", rapidjson::Value(brightness), alloc);
    val.AddMember("new_brightness", rapidjson::Value(new_brightness), alloc);
    return val;
  }
  std::string help() const {
    std::stringstream ss;
    ss << "  Arguments:\n";
    ss << "    incr:[AMOUNT}: increase brightness by amount\n";
    ss << "    set:[VALUE}: set brightness to value\n";
    return ss.str();
  }
};

void DisplayCommandGroup::registerCommands(Context* context,
    Command* command) {
  (*command)
    .setName("display")
    .setDescription("Display control");
  (*command->addChild())
    .setName("brightness").setName("bri")
    .setDescription("Change brightness")
    .makeAction<BrightnessChangeAction>(context);
}
