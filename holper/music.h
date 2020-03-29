#pragma once
#include <systemd/sd-bus.h>

class SDBus
{
public:
  void call(const char* service, const char* object, const char* iface, const char* method) {
  }
};

#define SPOTIFY_SERVICE "org.mpris.MediaPlayer2.spotify"
#define SPOTIFY_OBJECT "/org/mpris/MediaPlayer2"
#define SPOTIFY_IFACE "org.mpris.MediaPlayer2.Player"

class SpotifyAction : public Action
{
private:
  std::string method_;
public:
  SpotifyAction(std::shared_ptr<Context> context, std::string method)
      : Action(context), method_(method) {}
  std::string act(boost::program_options::variables_map vm) const
  {
    sd_bus* bus = nullptr;
    int r;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = nullptr;
    // TODO: generic sdbus code?
    r = sd_bus_open_user(&bus);
    if (r < 0) {
      char errbuf[1024];
      strerror_r(-r, errbuf, 1024);
      // TODO: play and next should spawn spotify here
      context_->logger->log(Logger::ERROR, "Failed to connect to bus: %s", errbuf);
      sd_bus_unref(bus);
      // TODO: throw
      return "failed";
    }
    r = sd_bus_call_method(bus, SPOTIFY_SERVICE, SPOTIFY_OBJECT, SPOTIFY_IFACE,
        method_.c_str(), &error, &m, "");
    if (r < 0) {
      context_->logger->log(Logger::ERROR, "Failed to call %s: %s", method_.c_str(), error.message);
      // TODO: dup code
      sd_bus_error_free(&error);
      sd_bus_message_unref(m);
      sd_bus_unref(bus);
      return "failed";
    }
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
    sd_bus_unref(bus);
    // TODO: return metadata for next/prev?
    return method_ + " succeeded";
  }
};

