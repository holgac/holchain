#pragma once
#include <unistd.h>
#include "sdbus.h"



class SpotifyAction : public Action
{
private:
  const char* kSpotifyService = "org.mpris.MediaPlayer2.spotify";
  const char* kSpotifyObject = "/org/mpris/MediaPlayer2";
  const char* kSpotifyIFace = "org.mpris.MediaPlayer2.Player";
  const char* kSpotifyLocation = "/usr/bin/spotify";
  std::string method_;
  bool spawnOnFailure_;
public:
  SpotifyAction(std::shared_ptr<Context> context, std::string method, bool spawn = false)
      : Action(context), method_(method), spawnOnFailure_(spawn) {}
  std::string act(boost::program_options::variables_map vm) const
  {
    SDBus bus(kSpotifyService, kSpotifyObject, kSpotifyIFace);
    try {
      bus.call(method_.c_str());
    } catch(HSDBusException& e) {
      if (!spawnOnFailure_ || strcmp(e.errorName(), SD_BUS_ERROR_SERVICE_UNKNOWN) != 0) {
        context_->logger->log(Logger::ERROR, e.what());
        return e.what();
      }
      context_->logger->log(Logger::INFO, "Spawning a new spotify instance");
      pid_t pid;
      if((pid = fork()) == 0) {
        // TODO: reset permissions and caps and stuff just in case
        // TODO: Helpers for fork+exec
        setsid();
        for(int i = getdtablesize(); i>=0; --i) {
          close(i);
        }
        int i = open("/dev/null", O_RDWR);
        dup(i);
        dup(i);
        umask(027);
        execl(kSpotifyLocation, kSpotifyLocation, NULL);
      }
      context_->logger->log(Logger::INFO, "Spotify pid: %ld", (long)pid);
      unsigned retries = 11;
      const useconds_t sleep_interval = 100000;
      while(--retries) {
        usleep(sleep_interval);
        context_->logger->log(Logger::INFO,
            "Spotify trying to %s again (%u retries left)", method_.c_str(), retries-1);
        try {
          bus.call(method_.c_str());
          break;
        } catch(HSDBusException& e) {
          if (strcmp(e.errorName(), SD_BUS_ERROR_SERVICE_UNKNOWN) != 0) {
            throw HSDBusException(std::move(e));
          }
        }
      }
      if (!retries) {
        return e.what();
      }
    }
    return method_ + " succeeded";
  }
};

