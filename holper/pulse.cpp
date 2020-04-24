#include "pulse.h"
#include "holper.h"
#include "command.h"
#include "consts.h"
#include "context.h"
#include "string.h"
#include "request.h"
#include "logger.h"
#include "workpool.h"
#include <pulse/pulseaudio.h>
#include <algorithm>
void pulse_callback(pa_context *context, void *userdata);

enum class PulseState
{
  CONNECTING,
  CONNECTED,
  ERROR,
};

class PulseAudio
{
private:
  PulseState state_;
  pa_mainloop* mainloop_;
  pa_context* pulseContext_;
  std::string defaultSinkName_;
  const pa_sink_info* defaultSink_;
  Context* context_;
  static void pulseSuccessCallback(pa_context* UNUSED(c), int UNUSED(success),
      void* UNUSED(p)) {}
  static void pulseServerInfoCallback(pa_context* UNUSED(c),
      const pa_server_info *i, void* p) {
    reinterpret_cast<PulseAudio*>(p)->defaultSinkName_ = i->default_sink_name;
  }

  static void pulseContextStateCallback(pa_context *context, void* userdata) {
    PulseAudio* pa = reinterpret_cast<PulseAudio*>(userdata);
    // TODO: call pa->callback(context)
    pa_context_state_t state = pa_context_get_state(context);
    switch(state) {
      default:
        pa->context_->logger->debug("What's %d?\n", (int)state);
        return;
      case PA_CONTEXT_UNCONNECTED:
        pa->context_->logger->debug("PulseAudio state: PA_CONTEXT_UNCONNECTED");
        break;
      case PA_CONTEXT_CONNECTING:
        pa->context_->logger->debug("PulseAudio state: PA_CONTEXT_CONNECTING");
        break;
      case PA_CONTEXT_AUTHORIZING:
        pa->context_->logger->debug("PulseAudio state: PA_CONTEXT_AUTHORIZING");
        break;
      case PA_CONTEXT_SETTING_NAME:
        pa->context_->logger->debug("PulseAudio state: PA_CONTEXT_SETTING_NAME");
        break;
      case PA_CONTEXT_TERMINATED:
        pa->context_->logger->debug("PulseAudio state: PA_CONTEXT_TERMINATED");
        break;
      case PA_CONTEXT_READY:
        pa->context_->logger->debug("PulseAudio state: PA_CONTEXT_READY");
        pa->state_ = PulseState::CONNECTED;
        break;
      case PA_CONTEXT_FAILED:
        pa->context_->logger->debug("PulseAudio state: PA_CONTEXT_FAILED");
        pa->logError("failed to connect");
        pa->state_ = PulseState::ERROR;
        break;
    }
  }
  static void pulseSinkInfoCallback(pa_context* UNUSED(c),
      const pa_sink_info* info, int eol, void* userdata) {
    if (eol) {
      return;
    }
    PulseAudio* pa = reinterpret_cast<PulseAudio*>(userdata);
    if(pa->defaultSinkName_ != info->name) {
      pa->logError("Received an irrelevant sink");
      // TODO: error handling
      return;
    }
    pa->defaultSink_ = info;
  }
  void logError(const char* msg) {
      context_->logger->error("%s: %s",
          msg, pa_strerror(pa_context_errno(pulseContext_)));
  }
  void iterate(pa_operation* op) {
    int ret;
    if (op == nullptr) {
      while(state_ == PulseState::CONNECTING) {
        if(pa_mainloop_iterate(mainloop_, 1, &ret) < 0) {
          logError("Failed to iterate on connect");
          break;
        }
      }
    } else {
      while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
        if(pa_mainloop_iterate(mainloop_, 1, &ret) < 0) {
          logError("Failed to iterate");
          break;
        }
      }
      pa_operation_unref(op);
    }
  }
public:
  PulseAudio(Context* context) : context_(context) {
  }
  ~PulseAudio() {
    pa_context_disconnect(pulseContext_);
    pa_context_unref(pulseContext_);
    pa_mainloop_free(mainloop_);
  }
  void init() {
    state_ = PulseState::CONNECTING;
    mainloop_ = pa_mainloop_new();
    if (!mainloop_) {
      logError("mainloop failed");
      // TODO: throw
      return;
    }
    pa_mainloop_api* api = pa_mainloop_get_api(mainloop_);
    if (!api) {
      logError("api failed");
      return;
    }
    pulseContext_ = pa_context_new(api, "holper");
    if (!pulseContext_) {
      logError("context failed");
      return;
    }
    pa_context_set_state_callback(pulseContext_, pulseContextStateCallback, this);
    if (pa_context_connect(pulseContext_, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
      logError("context connect failed");
      return;
    }
    iterate(nullptr);
    if (state_ == PulseState::ERROR) {
      logError("failed to connect\n");
      return;
    }
    iterate(pa_context_get_server_info(pulseContext_, pulseServerInfoCallback, this));
    iterate(pa_context_get_sink_info_by_name(pulseContext_, defaultSinkName_.c_str(), pulseSinkInfoCallback, this));
  }

  float getVolume() {
    return pa_cvolume_avg(&defaultSink_->volume) * 1.0f / 65536;
  }
  void setVolume(float vol_perc) {
    if (vol_perc < 0.0f) {
      vol_perc = 0.0f;
    } else if (vol_perc > 2.0f) {
      vol_perc = 2.0f;
    }
    pa_cvolume cvol;
    pa_cvolume_set(&cvol, defaultSink_->volume.channels,
        (pa_volume_t)(vol_perc * 65536));
    iterate(pa_context_set_sink_volume_by_name(pulseContext_,
        defaultSinkName_.c_str(), &cvol, pulseSuccessCallback, this));
  }
  void setMute(bool mute) {
    iterate(pa_context_set_sink_mute_by_name(pulseContext_, defaultSinkName_.c_str(),
      (int)mute, pulseSuccessCallback, this));
  }
  bool isMuted() {
    return defaultSink_->mute;
  }
  void toggleMute() {
    return setMute(!defaultSink_->mute);
  }
};

class VolumeAction : public Action
{
public:
  void spec(ParamSpec& spec) const override {
    spec
      .param<int>("set", "operations", "sets volume to value")
      .param<int>("incr", "operations", "increments volume by value")
      .param<std::nullptr_t>("mute", "operations", "Toggles mute/unmute")
      .key("operations", 1, 1);
  }

  rapidjson::Value actOn(Work* work) const override {
    auto& params = work->parameters();
    PulseAudio pa(context_);
    pa.init();
    rapidjson::Value val(rapidjson::kObjectType);
    float vol = pa.getVolume();
    bool mute = pa.isMuted();
    if (params.get<nullptr_t>("mute")) {
      pa.toggleMute();
      val.AddMember("volume", rapidjson::Value(vol), work->allocator());
      val.AddMember("old_mute", rapidjson::Value(mute), work->allocator());
      val.AddMember("new_mute", rapidjson::Value(!mute), work->allocator());
      return val;
    }
    float new_vol;
    if (auto incr = params.get<int>("incr")) {
      new_vol = vol + *incr * 0.01f;
    } else if (auto set = params.get<int>("set")) {
      new_vol = *set * 0.01f;
    }
    new_vol = std::clamp(new_vol, 0.0f, 2.0f);
    pa.setVolume(new_vol);
    val.AddMember("old_volume",
        rapidjson::Value((int)(100 * vol)),
        work->allocator());
    val.AddMember("new_volume",
        rapidjson::Value((int)(100 * new_vol)),
        work->allocator());
    val.AddMember("mute",
        rapidjson::Value(mute),
        work->allocator());
    return val;
  }

  VolumeAction(Context* context) : Action(context) {}
};

void PulseCommandGroup::initializeCommand(Context* context,
      Command* command)
{
  (*command)
    .setName("volume").setName("vol")
    .setDescription("Volume management")
    .makeAction<VolumeAction>(context);
}
