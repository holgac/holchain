#include "holper.h"
#include "string.h"
#include "socket.h"
#include "request.h"
#include "context.h"
#include "logger.h"
#include "exception.h"
#include "resolver.h"
#include "responder.h"
#include "commandmanager.h"
#include "command.h"
#include "workpool.h"
#include "info.h"
#include "display.h"
#include "pulse.h"
#include "music.h"
#include "system.h"
#include <memory>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <systemd/sd-daemon.h>

class Server
{
  std::unique_ptr<UnixSocket> socket_;
  Context* context_;
  void validateSocketPath(const std::string path) {
    struct stat statbuf;
    if (0 != stat(path.c_str(), &statbuf)) {
      context_->logger->info("%s does not exist, will create", path.c_str());
    } else {
      if(0 != unlink(path.c_str())) {
        THROW("Unlink failed: %s", StringUtils::errorString().c_str());
      }
      context_->logger->info("%s already exists, unlinked", path.c_str());
    }
  }
public:
  Server(Context* context, const std::string& path)
      : context_(context) {
    validateSocketPath(path);
    socket_ = std::make_unique<UnixSocket>(context_, path);
  }
  void accept(std::unique_ptr<UnixSocket> socket) {
    std::unique_ptr<Request> request(new Request(context_, std::move(socket)));
    context_->logger->debug("Server received new request: %d", request->id());
    context_->resolver->sendMessage(
        std::make_unique<ResolverArgs>(std::move(request)));
  }
  void serve() {
    socket_->serve(std::bind(&Server::accept, this, std::placeholders::_1));
  }
};

int main(int argc, char** argv) {
  Context context;
  int opt;
  std::string socket_path = St::fmt(
      "/run/user/%d/holperd.sock", getuid());
  std::string dev_socket_path = St::fmt(
      "/run/user/%d/holperdev.sock", getuid());
  bool verbose = false;
  size_t worker_threads = 4;
  bool dev_mode = false;
  auto print_help_and_exit = [&](int code) {
    printf("Holper server - System control helper\n");
    printf("Options:\n");
    printf("  -h: Print this message and exit\n");
    printf("  -d: Development mode (socket path: %s and verbose)\n",
        dev_socket_path.c_str());
    printf("  -s [PATH]: Use provided unix socket path (current: %s)\n",
        socket_path.c_str());
    printf("  -v: Verbose\n");
    printf("  -w [COUNT]: Worker thread count (current: %lu)\n",
        worker_threads);
    exit(code);
  };
  while ((opt = getopt(argc, argv, "+ds:vw:h")) != -1) {
    switch(opt) {
      case 'd':
        socket_path = dev_socket_path;
        verbose = true;
        dev_mode = true;
        break;
      case 's':
        socket_path = optarg;
        break;
      case 'v':
        verbose = true;
        break;
      case 'w':
        if (1 != sscanf(optarg, "%lu", &worker_threads)) {
          print_help_and_exit(-1);
        }
        break;
      case 'h':
      default:
        print_help_and_exit(opt == 'h' ? 0 : -1);
    }
  }
  context.logger.reset(new Logger(verbose ? Logger::DEBUG : Logger::MUSTFIX));
  context.logger->addTarget(
      std::make_unique<FDLogTarget>(STDOUT_FILENO, false));
  std::string logfile;
  if (dev_mode) {
    logfile = St::fmt("/run/user/%d/holperdev.log", getuid());
  } else {
    logfile = St::fmt("/run/user/%d/holperd.log", getuid());
  }
  int logfd = open(logfile.c_str(), O_APPEND | O_CREAT | O_SYNC | O_WRONLY,
      S_IRUSR | S_IWUSR);
  if (logfd < 0) {
    context.logger->logErrno("Log file %s failed to open", logfile.c_str());
    THROW("Log file %s failed to open", logfile.c_str());
  }
  context.logger->addTarget(std::make_unique<FDLogTarget>(logfd, true));
  context.resolver.reset(new Resolver(&context));
  context.resolver->start();
  context.responder.reset(new Responder(&context));
  context.responder->start();
  context.commandManager.reset(new CommandManager(&context));
  context.commandManager->registerCommandGroup<InfoCommandGroup>();
  context.commandManager->registerCommandGroup<DisplayCommandGroup>();
  context.commandManager->registerCommandGroup<PulseCommandGroup>();
  context.commandManager->registerCommandGroup<MusicCommandGroup>();
  context.commandManager->registerCommandGroup<SystemCommandGroup>();
  context.workPool.reset(new WorkPool(&context, worker_threads));
  context.workPool->start();
  Server server(&context, socket_path);
  sd_notify(0, "READY=1");
  server.serve();
  return 0;
}
