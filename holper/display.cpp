#include "display.h"
#include "holper.h"
#include "command.h"
#include "context.h"
#include "logger.h"
#include "string.h"
#include "request.h"
#include "exception.h"
#include "workpool.h"
#include "filesystem.h"
#include <algorithm>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/dpms.h>

class BrightnessChangeAction : public Action {
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
    } else if (opcnt > 1) {
      return "Got multiple operations";
    }
    return std::nullopt;
  }
  rapidjson::Value actOn(Work* work) const override {
    auto& params = work->parameters();
    // TODO: helpers for reading sys files
    int max_raw_brightness, raw_brightness;
    Fs::parse(kBrightnessPath + "/max_brightness", "%d", &max_raw_brightness);
    Fs::parse(kBrightnessPath + "/brightness", "%d", &raw_brightness);
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
    } else if (new_brightness < .001f && new_brightness < (brightness + 0.001f)) {
      new_brightness = 0.0f;
      setScreenPower(false);
    }
    raw_brightness = new_brightness * max_raw_brightness;
    Fs::dump(kBrightnessPath + "/brightness", "%d", raw_brightness);
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
      "    incr:[AMOUNT]: increase brightness by amount\n"
      "    set:[VALUE]: set brightness to value\n";
  }
};

class KeyboardBacklightAction : public Action {
  const std::string kPath = "/sys/class/leds/tpacpi::kbd_backlight";
public:
  KeyboardBacklightAction(Context* context) : Action(context) {}
protected:
  std::optional<std::string> failReason(Work* work) const override {
    auto& params = work->parameters();
    int opcnt = 0;
    bool exists;
    // TODO: validator
    if (!params.get<std::nullptr_t>("on", &exists) && exists) {
      return St::fmt("on parameter has value: %s",
          params.get<std::string>("on")->c_str());
    }
    opcnt += (int)exists;
    if (!params.get<std::nullptr_t>("off", &exists) && exists) {
      return St::fmt("off parameter has value: %s",
          params.get<std::string>("off")->c_str());
    }
    opcnt += (int)exists;
    if (!params.get<std::nullptr_t>("toggle", &exists) && exists) {
      return St::fmt("toggle parameter has value: %s",
          params.get<std::string>("toggle")->c_str());
    }
    opcnt += (int)exists;
    if (!params.get<int>("set", &exists) && exists) {
      return St::fmt("set value not int: %s",
          params.get<std::string>("set")->c_str());
    }
    opcnt += (int)exists;
    if (!params.get<int>("incr", &exists) && exists) {
      return St::fmt("incr value not int: %s",
          params.get<std::string>("incr")->c_str());
    }
    opcnt += (int)exists;
    if (opcnt == 0) {
      return "No operation was specified";
    } else if (opcnt > 1) {
      return "Got multiple operations";
    }
    return std::nullopt;
  }
  rapidjson::Value actOn(Work* work) const override {
    auto& params = work->parameters();
    int max_brightness, brightness;
    Fs::parse(kPath + "/max_brightness", "%d", &max_brightness);
    Fs::parse(kPath + "/brightness", "%d", &brightness);
    rapidjson::Value val(rapidjson::kObjectType);
    val.AddMember("old_brightness",
        rapidjson::Value(brightness),
        work->allocator());
    if (params.get<std::nullptr_t>("on")) {
      brightness = max_brightness;
    } else if (params.get<std::nullptr_t>("off")) {
      brightness = 0;
    } else if (params.get<std::nullptr_t>("toggle")) {
      brightness = brightness ? 0 : max_brightness;
    } else if (auto val = params.get<int>("set")) {
      brightness = std::clamp(*val, 0, max_brightness);
    } else if (auto val = params.get<int>("incr")) {
      brightness += *val;
      brightness = std::clamp(brightness, 0, max_brightness);
    }
    Fs::dump(kPath + "/brightness", "%d", brightness);
    val.AddMember("new_brightness",
        rapidjson::Value(brightness),
        work->allocator());
    return val;
  }

  std::string help() const override {
    return "  Arguments:\n"
      "    on: turns keyboard backlight on\n"
      "    off: turns keyboard backlight off\n"
      "    toggle: toggles keyboard backlight\n"
      "    set:[VALUE]: set keyboard backlight to value\n";
  }
};

void DisplayCommandGroup::initializeCommand(Context* context,
    Command* command) {
  (*command)
    .setName("display").setName("visual")
    .setDescription("Display control");
  (*command->addChild())
    .setName("brightness").setName("bri")
    .setDescription("Change brightness")
    .makeAction<BrightnessChangeAction>(context);
  (*command->addChild())
    .setName("keyboard_backlight").setName("kbl")
    .setDescription("Keyboard backlight operations")
    .makeAction<KeyboardBacklightAction>(context);
}
