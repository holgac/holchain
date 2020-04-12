#pragma once
#include "request.h"
#include "thread.h"
#include "context.h"
#include <memory>


struct ResponderArgs {
  std::unique_ptr<Request> request;
  int code;
  rapidjson::Value response;
  ResponderArgs(std::unique_ptr<Request> req, rapidjson::Value& resp,
      int returnCode
  ) : request(std::move(req)), code(returnCode), response(std::move(resp)) {}
};

class Responder : public Thread<ResponderArgs>
{
public:
  explicit Responder(Context* context) : Thread("Responder", context) {}
  void handleMessage(std::unique_ptr<ResponderArgs> msg) override;
};

