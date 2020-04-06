#include "workpool.h"
#include "holper.h"
#include "logger.h"
#include "commandmanager.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

void WorkPoolWorker::run() {
  running_ = true;
  while (running_) {
    auto work = workPool_->getWork();
    context_->logger->log(Logger::INFO, "Will process work of fd %d", work->socketFd);
    std::string result = context_->commandManager->runCommand(work->args);
    context_->logger->log(Logger::INFO, "Sending response: %s%s%s",
        Consts::TerminalColors::PURPLE,
        result.c_str(),
        Consts::TerminalColors::DEFAULT);
    boost::property_tree::ptree tree;
    tree.put("code", 0);
    tree.put("response", result);
    std::stringstream out;
    boost::property_tree::write_json(out, tree, false);
    auto outstr = out.str();
    write(work->socketFd, outstr.c_str(), outstr.size());
    close(work->socketFd);
  }
}

void WorkPool::handleMessage(std::unique_ptr<WorkPoolArgs> msg) {
  std::unique_ptr<Work> work(new Work(msg->socketFd, msg->args));
  {
    LockMutex lock(&workMutex_);
    works_.push(std::move(work));
  }
  context_->logger->log(Logger::INFO, "Received work of fd %d", msg->socketFd);
  sem_post(&workSemaphore_);
}

std::unique_ptr<WorkPool::Work> WorkPool::getWork() {
  sem_wait(&workSemaphore_);
  std::unique_ptr<Work> work;
  {
    LockMutex lock(&workMutex_);
    work = std::move(works_.front());
    works_.pop();
  }
  return work;
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
