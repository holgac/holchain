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
#include <typeinfo>

class BrightnessChangeAction : public Action {
  const std::string kBrightnessPath = "/sys/class/backlight/amdgpu_bl0";
  void setScreenPower(bool val) const {
    context_->logger->info("Turning display %s", val ? "on" : "off");
    Display *dpy = XOpenDisplay(NULL);
    int dummy;
    if (!DPMSQueryExtension(dpy, &dummy, &dummy)) {
      context_->logger->error("no DPMS extension");
      XCloseDisplay(dpy);
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

  void spec(ParamSpec& spec) const override {
    spec
      .param<int>("set", "operations", "sets brightness to value")
      .param<int>("incr", "operations", "increments brightness by value")
      .key("operations", 1, 1);
  }
};

class KeyboardBacklightAction : public Action {
  const std::string kPath = "/sys/class/leds/tpacpi::kbd_backlight";
public:
  KeyboardBacklightAction(Context* context) : Action(context) {}
  void spec(ParamSpec& spec) const override {
    auto help = [](const std::string& pre, const std::string& post) {
      if (post.empty()) {
        return pre + " keyboard backlight";
      }
      return pre + " keyboard backlight " + post;
    };
    spec
      .param<std::nullptr_t>("on", "operations", help("turns", "on"))
      .param<std::nullptr_t>("off", "operations", help("turns", "off"))
      .param<std::nullptr_t>("toggle", "operations", help("toggles", ""))
      .param<int>("set", "operations", help("sets", "to value"))
      .param<int>("incr", "operations", help("increments ", "by value"))
      .key("operations", 1, 1);
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
