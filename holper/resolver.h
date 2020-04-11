#pragma once
#include "request.h"
#include "thread.h"
#include "context.h"
#include <memory>


struct ResolverArgs {
  std::unique_ptr<Request> request;
  explicit ResolverArgs(std::unique_ptr<Request>& req)
      : request(std::move(req)) {}
};

class Resolver : public Thread<ResolverArgs>
{
public:
  explicit Resolver(Context* context) : Thread("Resolver", context) {}
  void handleMessage(std::unique_ptr<ResolverArgs> msg) override;
};
