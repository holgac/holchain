#include "workpool.h"
#include "holper.h"
#include "logger.h"
#include "commandmanager.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// TODO: move to request.cpp
int Request::idCounter_;

void WorkPoolWorker::run() {
  running_ = true;
  while (running_) {
    auto request = workPool_->getWork();
    context_->logger->log(Logger::INFO, "Will process request %d", request->id());
    // TODO: we should have Action/Command etc. here. CommandManager stuff should
    // run before here, probably in parser
    std::string result = request->command()->getAction()->actOn(request.get());
    context_->logger->log(Logger::INFO, "Sending response: %s%s%s",
        Consts::TerminalColors::PURPLE,
        result.c_str(),
        Consts::TerminalColors::DEFAULT);
    request->response()
      .set("response", result)
      .set("code", 0);
    request->respond();
  }
}

void WorkPool::handleMessage(std::unique_ptr<WorkPoolArgs> msg) {
  context_->logger->log(Logger::INFO, "Received work for request %d",
      msg->request->id());
  std::unique_ptr<Work> work(new Work(std::move(msg->request)));
  {
    LockMutex lock(&workMutex_);
    works_.push(std::move(work));
  }
  sem_post(&workSemaphore_);
}

std::unique_ptr<Request> WorkPool::getWork() {
  sem_wait(&workSemaphore_);
  std::unique_ptr<Work> work;
  {
    LockMutex lock(&workMutex_);
    work = std::move(works_.front());
    works_.pop();
  }
  return std::move(work->request);
}
void WorkPool::init() {
  for (size_t i=0; i<poolSize_; ++i) {
    char thread_name[32];
    sprintf(thread_name, "Worker%u", (unsigned)i);
    WorkPoolWorker* worker = new WorkPoolWorker(thread_name, context_, this);
    worker->start();
    workers_.push_back(std::unique_ptr<WorkPoolWorker>(worker));
  }
}
