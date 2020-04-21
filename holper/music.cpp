#include "music.h"
#include "holper.h"
#include "sdbus.h"
#include "command.h"
#include "logger.h"
#include "context.h"
#include "string.h"
#include "workpool.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

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
  SpotifyAction(Context* context, std::string method, bool spawn = false)
      : Action(context), method_(method), spawnOnFailure_(spawn) {}

protected:
  std::optional<std::string> failReason(Work* work) const override {
    if (!work->parameters().map().empty()) {
      return method_ + " takes no parameter";
    }
    return std::nullopt;
  }
  rapidjson::Value actOn(Work* UNUSED(work)) const override {
    SDBus bus(kSpotifyService, kSpotifyObject, kSpotifyIFace, true);
    try {
      bus.call(method_.c_str());
    } catch(HSDBusException& e) {
      if (!spawnOnFailure_ || strcmp(e.errorName(), SD_BUS_ERROR_SERVICE_UNKNOWN) != 0) {
        throw;
      }
      context_->logger->info("Spawning a new spotify instance");
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
      context_->logger->info("Spotify pid: %ld", (long)pid);
      unsigned retries = 11;
      const useconds_t sleep_interval = 100000;
      while(--retries) {
        usleep(sleep_interval);
        context_->logger->info(
            "Spotify trying to %s again (%u retries left)",
            method_.c_str(), retries-1);
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
        throw;
      }
    }
    return rapidjson::Value("Success");
  }
  std::string help() const override {
    return "";
  }
};


void MusicCommandGroup::initializeCommand(Context* context, Command* command) {
  (*command)
    .setName("music").setName("mus")
    .setDescription("Music control");
  (*command->addChild())
    .setName("play")
    .setDescription("Play music")
    .makeAction<SpotifyAction>(context, "Play", true);
  (*command->addChild())
    .setName("pause")
    .setDescription("Pause music")
    .makeAction<SpotifyAction>(context, "Pause");
  (*command->addChild())
    .setName("playpause").setName("p")
    .setDescription("Toggle play/pause music")
    .makeAction<SpotifyAction>(context, "PlayPause", true);
  (*command->addChild())
    .setName("next").setName("n")
    .setDescription("Next song")
    .makeAction<SpotifyAction>(context, "Next");
  (*command->addChild())
    .setName("prev").setName("previous")
    .setDescription("Previous song")
    .makeAction<SpotifyAction>(context, "Previous");
}
