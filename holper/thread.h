#pragma once
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <string>
#include <memory>
#include "exception.h"
#include "string.h"

class Context;

class LockMutex {
private:
  pthread_mutex_t* mutex_;
public:
  LockMutex(pthread_mutex_t* mutex) : mutex_(mutex) {
    int r = pthread_mutex_lock(mutex_);
    if (r != 0) {
      THROW("Mutex lock failed: %s", StringUtils::errorString(r));
    }
  }
  ~LockMutex() {
    pthread_mutex_unlock(mutex_);
  }
};

class ThreadBase
{
protected:
  std::string name_;
  virtual void run() = 0;
  Context* context_;
public:
  ThreadBase(std::string name, Context* context)
      : name_(name), context_(context) {}
  void start() {
    pthread_t tmp;
    int r = pthread_create(&tmp, NULL, startThread, (void*)this);
    if (r != 0) {
      THROW("Thread creation failed: %s", StringUtils::errorString(r));
    }
  }
  static void* startThread(void* thread_ptr);
};

template <typename T>
class Thread : public ThreadBase
{
private:
  std::queue<std::unique_ptr<T>> messages_;
  pthread_t thread_;
  pthread_mutex_t messagesMutex_;
  sem_t messagesSemaphore_;
public:
  Thread(std::string name, Context* context)
      : ThreadBase(name, context) {
    int r = pthread_mutex_init(&messagesMutex_, NULL);
    if (r != 0) {
      THROW("Mutex init failed: %s", StringUtils::errorString(r));
    }
    r = sem_init(&messagesSemaphore_, 0, 0);
    if (r != 0) {
      THROW("Semaphore init failed: %s", StringUtils::errorString(r));
    }
  }
  void sendMessage(std::unique_ptr<T> t) {
    {
      LockMutex lock(&messagesMutex_);
      messages_.push(std::move(t));
    }
    int r = sem_post(&messagesSemaphore_);
    if (r != 0) {
      THROW("Semaphore post failed: %s", StringUtils::errorString(r));
    }
  }
  virtual void init() {}
protected:
  void run() final {
    init();
    while(true) {
      int r = sem_wait(&messagesSemaphore_);
      if (r != 0) {
        THROW("Semaphore wait failed: %s", StringUtils::errorString(r));
      }
      std::unique_ptr<T> msg;
      {
        LockMutex lock(&messagesMutex_);
        msg = std::move(messages_.front());
        messages_.pop();
      }
      handleMessage(std::move(msg));
    }
  }
  virtual void handleMessage(std::unique_ptr<T> msg) = 0;
};


