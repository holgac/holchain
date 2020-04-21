#include "resolver.h"
#include "logger.h"
#include "string.h"
#include "commandmanager.h"
#include "command.h"
#include "workpool.h"
#include "responder.h"
#include <rapidjson/document.h>
#include <vector>
#include <string>
#include <map>

void Resolver::sendToResponder(Request* request,
    std::unique_ptr<WorkResult> result) {
  // This runs in worker threads
  context_->logger->info("Sending request %d to responder", request->id());
  request->profiler().join(result->work->profiler(), "Work.");
  context_->responder->sendMessage(std::make_unique<ResponderArgs>(
        std::unique_ptr<Request>(request),
        std::move(result->result.Move()),
        result->code));
}

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
  std::map<std::string, std::string> str_parameters;
  std::map<std::string, rapidjson::Value> raw_parameters;
  for ( auto& val : doc["parameters"].GetObject() ) {
    auto& name = val.name;
    auto& value = val.value;
    if (value.IsString()) {
      str_parameters.insert(std::make_pair<std::string, std::string>(
            std::move(std::string(name.GetString(), name.GetStringLength())),
            std::move(std::string(value.GetString(), value.GetStringLength()))));
    } else {
      raw_parameters.insert(std::make_pair<std::string, rapidjson::Value>(
        std::move(std::string(name.GetString(), name.GetStringLength())),
        std::move(value.Move())
      ));
    }
  }
  Parameters parameters(std::move(str_parameters), std::move(raw_parameters));
  request->profiler().event("Payload json parsed");
  auto work = std::make_unique<Work>(request->id(),
      context_->commandManager->resolveCommand(command_tokens),
      std::move(parameters),
      request->response().alloc(),
      std::bind(&Resolver::sendToResponder, this, request.get(),
          std::placeholders::_1));
  request->profiler().event("Resolved command");
  context_->logger->info("Request %d will run %s",
      request->id(),
      work->command()->name().c_str());
  context_->workPool->sendMessage(std::move(work));
  request.release();
}
