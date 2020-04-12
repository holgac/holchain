#include "resolver.h"
#include "logger.h"
#include "string.h"
#include "commandmanager.h"
#include "command.h"
#include "workpool.h"
#include <rapidjson/document.h>
#include <vector>
#include <string>
#include <map>

void Resolver::handleMessage(std::unique_ptr<ResolverArgs> msg) {
  std::unique_ptr<Request> request = std::move(msg->request);
  request->profiler().event("Received by Resolver");
  context_->logger->info("Resolver received request %d", request->id());
  std::string payload = request->socket()->read();
  rapidjson::Document doc;
  doc.Parse(payload.c_str());
  context_->logger->info("Request %d: %s", request->id(), payload.c_str());
  request->setVerbose(doc["verbose"].GetBool());
  std::vector<std::string> command_tokens;
  for ( auto& val : doc["command"].GetArray() ) {
    command_tokens.push_back(std::string(val.GetString(), val.GetStringLength()));
  }
  std::map<std::string, std::string> parameters;
  for ( auto& val : doc["parameters"].GetObject() ) {
    auto& name = val.name;
    auto& value = val.value;
    parameters.insert(std::make_pair<std::string, std::string>(
          std::move(std::string(name.GetString(), name.GetStringLength())),
          std::move(std::string(value.GetString(), value.GetStringLength()))));
  }
  request->profiler().event("Payload json parsed");
  request->setCommand(
      context_->commandManager->resolveCommand(command_tokens));
  request->setCommandTokens(std::move(command_tokens));
  request->setParameters(std::move(parameters));
  request->profiler().event("Resolved command");
  context_->logger->info("Request %d will run %s",
      request->id(),
      request->command()->name().c_str());
  context_->workPool->sendMessage(
      std::make_unique<WorkPoolArgs>(std::move(request)));
}
