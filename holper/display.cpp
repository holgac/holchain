#include "display.h"
#include <fstream>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/dpms.h>
#include "command.h"
#include "logger.h"

class BrightnessChangeAction : public Action
{
  const std::string kBrightnessPath = "/sys/class/backlight/amdgpu_bl0";
  void setScreenPower_(bool val) const {
    context_->logger->log(Logger::INFO, "Turning display %s", val?"on":"off");
    Display *dpy;
    dpy = XOpenDisplay(NULL);
    int dummy;
    if (!DPMSQueryExtension(dpy, &dummy, &dummy)) {
      context_->logger->log(Logger::INFO, "no DPMS extension");
      return;
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
  BrightnessChangeAction(std::shared_ptr<Context> context) : Action(context) {}
  virtual boost::optional<boost::program_options::options_description>
      options() const {
    boost::program_options::options_description desc;
    desc.add_options()
      ("set", "set instead of increase")
      ("value", boost::program_options::value<int>()->default_value(5),
       "Amount to increase brightness by");
    return boost::make_optional(desc);
  }
  virtual boost::optional<boost::program_options::positional_options_description>
      positionalOptions() const {
    boost::program_options::positional_options_description desc;
    desc.add("value", 5);
    return boost::make_optional(desc);
  }
  std::string act(boost::program_options::variables_map vm) const
  {
    // TODO: helpers for reading sys files
    int max_bri, bri;
    {
      std::ifstream max_bri_file(kBrightnessPath + "/max_brightness");
      max_bri_file >> max_bri;
    }
    {
      std::ifstream bri_file(kBrightnessPath + "/brightness");
      bri_file >> bri;
    }
    float brightness = bri * 1.0f / max_bri;
    float value = vm["value"].as<int>() * 0.01f;
    if (brightness < 0.001f && value > .0f) {
      setScreenPower_(true);
    }
    if (vm.count("set")) {
      brightness = value;
    } else {
      brightness += value;
    }
    if (brightness < .001f) {
      brightness = 0.0f;
      setScreenPower_(false);
    }
    if (brightness > 1.0f) {
      brightness = 1.0f;
    }
    {
      std::ofstream bri_file(kBrightnessPath + "/brightness");
      bri = brightness * max_bri;
      bri_file << bri;
    }
    return "success";
  }
};

void DisplayCommandGroup::registerCommands(std::shared_ptr<Context> context,
      std::shared_ptr<Command> command)
{
  command
    ->name("display")
    ->help("Display control");
  command->addChild()
    ->name("brightness")->name("bri")
    ->help("Change brightness")
    ->action(new BrightnessChangeAction(context));
}
