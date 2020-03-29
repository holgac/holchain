#pragma once
#include <pulse/pulseaudio.h>
void pulse_callback(pa_context *context, void *userdata);

enum class PulseState
{
  CONNECTING,
  CONNECTED,
  ERROR,
};

class PulseObject
{
};

class PulseAudio
{
private:
  PulseState state_;
  pa_mainloop* mainloop_;
  pa_context* pulseContext_;
  std::string defaultSinkName_;
  const pa_sink_info* defaultSink_;
  std::shared_ptr<Context> context_;
  static void pulseSuccessCallback(pa_context *c, int success, void* p) {
    printf("Success: %d\n", success);
  }
  static void pulseServerInfoCallback(pa_context *c, const pa_server_info *i, void* p) {
    reinterpret_cast<PulseAudio*>(p)->defaultSinkName_ = i->default_sink_name;
  }

  static void pulseContextStateCallback(pa_context *context, void* userdata) {
    PulseAudio* pa = reinterpret_cast<PulseAudio*>(userdata);
    // TODO: call pa->callback(context)
    pa_context_state_t state = pa_context_get_state(context);
    switch(state) {
      default: printf("What's %d?\n", (int)state); return;
      case PA_CONTEXT_UNCONNECTED:
        pa->context_->logger->log(Logger::DEBUG, "PulseAudio state: PA_CONTEXT_UNCONNECTED");
        break;
      case PA_CONTEXT_CONNECTING:
        pa->context_->logger->log(Logger::DEBUG, "PulseAudio state: PA_CONTEXT_CONNECTING");
        break;
      case PA_CONTEXT_AUTHORIZING:
        pa->context_->logger->log(Logger::DEBUG, "PulseAudio state: PA_CONTEXT_AUTHORIZING");
        break;
      case PA_CONTEXT_SETTING_NAME:
        pa->context_->logger->log(Logger::DEBUG, "PulseAudio state: PA_CONTEXT_SETTING_NAME");
        break;
      case PA_CONTEXT_TERMINATED:
        pa->context_->logger->log(Logger::DEBUG, "PulseAudio state: PA_CONTEXT_TERMINATED");
        break;
      case PA_CONTEXT_READY:
        pa->context_->logger->log(Logger::DEBUG, "PulseAudio state: PA_CONTEXT_READY");
        pa->state_ = PulseState::CONNECTED;
        break;
      case PA_CONTEXT_FAILED:
        pa->context_->logger->log(Logger::DEBUG, "PulseAudio state: PA_CONTEXT_FAILED");
        pa->logError("failed to connect");
        pa->state_ = PulseState::ERROR;
        break;
    }
  }
  static void pulseSinkInfoCallback(pa_context* c, const pa_sink_info* info, int eol, void* userdata) {
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
      context_->logger->log(Logger::ERROR, "%s: %s",
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
  PulseAudio(std::shared_ptr<Context> context) : context_(context) {
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

  void incrementVolume(double increment) {
    float old_vol_perc = pa_cvolume_avg(&defaultSink_->volume) * 1.0f / 65536;
    float vol_perc = old_vol_perc + increment;
    if (vol_perc < 0.0f) {
      vol_perc = 0.0f;
    } else if (vol_perc > 2.0f) {
      vol_perc = 2.0f;
    }
    context_->logger->log(Logger::INFO, "%s volume %3.2f->%3.2f", defaultSinkName_.c_str(),
        old_vol_perc, vol_perc);
    pa_cvolume cvol;
    pa_cvolume_set(&cvol, defaultSink_->volume.channels,
        (pa_volume_t)(vol_perc * 65536));
    iterate(pa_context_set_sink_volume_by_name(pulseContext_,
        defaultSinkName_.c_str(), &cvol, pulseSuccessCallback, this));
  }
  void setMute(bool mute) {
    context_->logger->log(Logger::INFO, "Setting %s %smute", defaultSinkName_.c_str(), mute?"":"un");
    iterate(pa_context_set_sink_mute_by_name(pulseContext_, defaultSinkName_.c_str(),
          (int)mute, pulseSuccessCallback, this));
  }
  void toggleMute() {
    return setMute(!defaultSink_->mute);
  }
};

class VolumeChangeAction : public Action
{
public:
  VolumeChangeAction(std::shared_ptr<Context> context) : Action(context) {}
  virtual boost::optional<boost::program_options::options_description>
      options() const {
    boost::program_options::options_description desc;
    desc.add_options()
      ("volume", boost::program_options::value<int>()->default_value(5),
       "Amount to increase volume by");
    return boost::make_optional(desc);
  }
  virtual boost::optional<boost::program_options::positional_options_description>
      positionalOptions() const {
    boost::program_options::positional_options_description desc;
    desc.add("volume", 1);
    return boost::make_optional(desc);
  }
  std::string act(boost::program_options::variables_map vm) const
  {
    int vol = vm["volume"].as<int>();
    PulseAudio pa(context_);;
    pa.init();
    pa.incrementVolume(0.01f * vol);
    return "success";
  }
};

class VolumeMuteAction : public Action
{
public:
  VolumeMuteAction(std::shared_ptr<Context> context) : Action(context) {}
  std::string act(boost::program_options::variables_map vm) const
  {
    PulseAudio pa(context_);;
    pa.init();
    pa.toggleMute();
    return "success";
  }
};
