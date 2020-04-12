#include "workpool.h"
#include "holper.h"
#include "logger.h"
#include "string.h"
#include "context.h"
#include "commandmanager.h"
#include "command.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

WorkPoolWorker::WorkPoolWorker(int id, Context* context, WorkPool* workPool)
    : ThreadBase(St::fmt("WorkPoolWorker%d", id), context),
      workPool_(workPool), id_(id) {}

void WorkPoolWorker::run() {
  while (true) {
    auto request = std::move(workPool_->getRequest());
    request->profiler().event("Received by WorkPoolWorker");
    context_->logger->info("Will process request %d", request->id());
    if (request->verbose()) {
      request->response()
        .set("worker", name_);
    }
    auto action = request->command()->action();
    if (action == nullptr) {
      request->response().set("response", request->command()->help());
      request->sendResponse(-1);
      continue;
    }
    rapidjson::Value res;
    try {
      auto reason = action->failReason(request->parameters());
      if (reason) {
        context_->logger->info(
          "Request %d would fail: %s%s%s",
          request->id(),
          Consts::TerminalColors::RED,
          reason->c_str(),
          Consts::TerminalColors::DEFAULT);
        request->response().set("response",
            *reason + "\n" + request->command()->help());
        request->sendResponse(-1);
        continue;
      }
      res = action->actOn(request->parameters(),
          request->response().alloc()).Move();
    } catch (std::exception& e) {
      context_->logger->info(
        "Request %d failed: %s%s%s",
        request->id(),
        Consts::TerminalColors::RED,
        e.what(),
        Consts::TerminalColors::DEFAULT);
      request->response().set("response", e.what());
      request->sendResponse(-1);
      continue;
    }
    request->response().set("response", res.Move());
    request->sendResponse(0);
  }
}

void WorkPool::handleMessage(std::unique_ptr<WorkPoolArgs> msg) {
  std::unique_ptr<Request> request = std::move(msg->request);
  request->profiler().event("Received by WorkPool");
  context_->logger->info("Received work for request %d", request->id());
  std::unique_ptr<Work> work(new Work(std::move(request)));
  {
    LockMutex lock(&workMutex_);
    works_.push(std::move(work));
  }
  sem_post(&workSemaphore_);
}

std::unique_ptr<Request> WorkPool::getRequest() {
  sem_wait(&workSemaphore_);
  std::unique_ptr<Work> work;
  {
    LockMutex lock(&workMutex_);
    work = std::move(works_.front());
    works_.pop();
  }
  // TODO: do some p50-p95 calculation for wait times
  context_->logger->info(
        "Request %d waited in queue for %s",
      work->request->id(),
      (TimePoint() - work->ctime).str().c_str());
  return std::move(work->request);
}

WorkPool::WorkPool(Context* context, size_t poolSize)
    : Thread("WorkPool", context) {
  for (size_t i=0; i<poolSize; ++i) {
    auto worker = std::make_unique<WorkPoolWorker>((int)i, context_, this);
    worker->start();
    workers_.push_back(std::move(worker));
  }
}
