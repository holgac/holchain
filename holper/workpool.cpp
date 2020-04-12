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

std::pair<rapidjson::Value, int> WorkPoolWorker::runSingle(Work* work) {
  work->profile("Received by WorkPoolWorker");
  context_->logger->info("Will process request %d", work->requestId);
  auto action = work->command->action();
  auto format_retval = [&] (const std::string& val, int code) {
    return std::make_pair(
        rapidjson::Value(val.c_str(), val.size(), *work->allocator),
        code);
  };
  if (action == nullptr) {
    return form_retval(work->command->help(), -1);
  }
  rapidjson::Value res;
  try {
    if (auto reason = action->failReason(*work->parameters)) {
      context_->logger->info(
        "Request %d would fail: %s%s%s",
        work->requestId,
        Consts::TerminalColors::RED,
        reason->c_str(),
        Consts::TerminalColors::DEFAULT);
      return form_retval(*reason + "\n" + work->command->help(), -1);
    }
    return std::make_pair(action->actOn(*work->parameters, *work->allocator),
        0);
  } catch (std::exception& e) {
    context_->logger->info(
      "Request %d failed: %s%s%s",
      work->requestId,
      Consts::TerminalColors::RED,
      e.what(),
      Consts::TerminalColors::DEFAULT);
    return form_retval(e.what(), -1);
  }
}

void WorkPoolWorker::run() {
  while (true) {
    auto work = std::move(workPool_->getWork());
    auto res = runSingle(work.get());
    work->finish(std::move(res.first.Move()), res.second);
 }
}

void WorkPool::handleMessage(std::unique_ptr<Work> msg) {
  msg->profile("Received by WorkPool");
  context_->logger->info("Received work for request %d", msg->requestId);
  auto work = std::make_unique<WorkInternal>(std::move(msg));
  {
    LockMutex lock(&workMutex_);
    works_.push(std::move(work));
  }
  sem_post(&workSemaphore_);
}

std::unique_ptr<Work> WorkPool::getWork() {
  sem_wait(&workSemaphore_);
  std::unique_ptr<WorkInternal> work;
  {
    LockMutex lock(&workMutex_);
    work = std::move(works_.front());
    works_.pop();
  }
  // TODO: do some p50-p95 calculation for wait times
  context_->logger->info(
        "Request %d waited in queue for %s",
      work->work->requestId,
      (TimePoint() - work->ctime).str().c_str());
  return std::move(work->work);
}

WorkPool::WorkPool(Context* context, size_t poolSize)
    : Thread("WorkPool", context) {
  for (size_t i=0; i<poolSize; ++i) {
    auto worker = std::make_unique<WorkPoolWorker>((int)i, context_, this);
    worker->start();
    workers_.push_back(std::move(worker));
  }
}
