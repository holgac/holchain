#include "thread.h"
#include "holper.h"
#include "logger.h"

void* ThreadBase::startThread(void* thread_ptr) {
  ThreadBase* thread = reinterpret_cast<ThreadBase*>(thread_ptr);
  pthread_setname_np(pthread_self(), thread->name_.c_str());
  thread->context_->logger->log(Logger::INFO, "Thread starting");
  thread->run();
  return NULL;
}
