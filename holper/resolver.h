#pragma once
#include "request.h"
#include "thread.h"
#include "context.h"
#include <memory>

class WorkResult;

struct ResolverArgs {
  std::unique_ptr<Request> request;
  explicit ResolverArgs(std::unique_ptr<Request> req)
      : request(std::move(req)) {}
};

class Resolver : public Thread<ResolverArgs>
{
private:
  void sendToResponder(Request* request,
      std::unique_ptr<WorkResult> result);
public:
  explicit Resolver(Context* context) : Thread("Resolver", context) {}
  ~Resolver() {}
  void handleMessage(std::unique_ptr<ResolverArgs> msg) override;
};
