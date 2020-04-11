#include "holper.h"
#include "string.h"
#include "socket.h"
#include "context.h"
#include "logger.h"
#include "profiler.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <unistd.h>

int main(int argc, char** argv) {
  Context context;
  Profiler profiler;
  int opt;
  std::string socket_path = St::fmt(
      "/run/user/%d/holperd.sock", getuid());
  std::string dev_socket_path = St::fmt(
      "/run/user/%d/holperdev.sock", getuid());
  bool client_verbose = false;
  bool server_verbose = false;
  std::string out;
  auto printHelpAndExit = [&](int code) {
    printf("Holper client - Forms commands and sends to holper server\n");
    printf("Options:\n");
    printf("  -h: Print this message and exit\n");
    printf("  -d: Development mode (socket path: %s and verbose)\n",
        dev_socket_path.c_str());
    printf("  -s [PATH]: Use provided unix socket path (default: %s)\n",
        socket_path.c_str());
    printf("  -V : Verbose (server) \n");
    printf("  -v: Verbose (client)\n");
    exit(code);
  };
  while ((opt = getopt(argc, argv, "+ds:o:vV:h")) != -1) {
    switch(opt) {
      case 'd':
        socket_path = dev_socket_path;
        server_verbose = true;
        break;
      case 's':
        socket_path = optarg;
        break;
      case 'v':
        client_verbose = true;
        break;
      case 'V':
        server_verbose = true;
        break;
      case 'h':
      default:
        printHelpAndExit(opt=='h' ? 0 : -1);
    }
  }
  context.logger.reset(new Logger(client_verbose ? Logger::DEBUG : Logger::MUSTFIX));
  context.logger->addTarget(new FDLogTarget(STDERR_FILENO, false));
  profiler.event("Read program options");
  rapidjson::Document document;
  auto& alloc = document.GetAllocator();
  rapidjson::Value root(rapidjson::kObjectType);
  root.AddMember("verbose", rapidjson::Value(server_verbose), alloc);
  rapidjson::Value command(rapidjson::kArrayType);
  int i;
  for(i=optind; i<argc; ++i) {
    if (strcmp(argv[i], "--") == 0) {
      i++;
      break;
    }
    command.PushBack(rapidjson::Value(argv[i], strlen(argv[i])), alloc); 
  }
  root.AddMember("command", command, alloc);
  rapidjson::Value parameters(rapidjson::kObjectType);
  for(; i<argc; ++i) {
    char* col = strchr(argv[i], ':');
    if (col == nullptr) {
      parameters.AddMember(
          rapidjson::Value(argv[i], strlen(argv[i])),
          rapidjson::Value(""),
          alloc);
    } else {
      size_t cidx = col - argv[i];
      parameters.AddMember(
        rapidjson::Value(argv[i], cidx),
        rapidjson::Value(argv[i] + cidx + 1, strlen(argv[i]) - cidx - 1),
        alloc);
    }
  }
  root.AddMember("parameters", parameters, alloc);

  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
  root.Accept(writer);
  context.logger->info("Sending %s%s%s to %s",
        Consts::TerminalColors::PURPLE,
        buf.GetString(),
        Consts::TerminalColors::DEFAULT,
        socket_path.c_str());
  profiler.event("Constructed payload");
  UnixSocket sock(&context, socket_path);
  sock.connect();
  sock.write(std::string(buf.GetString(), buf.GetSize()));
  profiler.event("Sent data");
  auto response = sock.read();
  profiler.event("Received data");
  context.logger->info("Received response: %s%s%s",
        Consts::TerminalColors::YELLOW,
        response.c_str(),
        Consts::TerminalColors::DEFAULT
        );
  document.Parse(response.c_str());
  context.logger->info(profiler.str().c_str());
  if (client_verbose) {
    printf("%s\n", response.c_str());
  } else if (document["response"].IsString()) {
    printf("%s\n", document["response"].GetString());
  } else {
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    document["response"].Accept(writer);
    printf("%s", buf.GetString());
  }
  return document["code"].GetInt();
}
