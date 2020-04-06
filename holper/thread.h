#pragma once
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <string>
#include <memory>
class Context;

class LockMutex {
private:
  pthread_mutex_t* mutex_;
public:
  LockMutex(pthread_mutex_t* mutex) : mutex_(mutex) {
    pthread_mutex_lock(mutex_);
  }
  ~LockMutex() {
    pthread_mutex_unlock(mutex_);
  }
};

class ThreadBase
{
private:
  std::string name_;
protected:
  virtual void run() = 0;
  std::shared_ptr<Context> context_;
public:
  ThreadBase(std::string name, std::shared_ptr<Context> context)
      : name_(name), context_(context) {}
  void start() {
    pthread_t tmp;
    pthread_create(&tmp, NULL, startThread, (void*)this);
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
  bool running_;
public:
  Thread(std::string name, std::shared_ptr<Context> context)
      : ThreadBase(name, context) {}
  void sendMessage(T* t) {
    {
      LockMutex lock(&messagesMutex_);
      messages_.push(std::unique_ptr<T>(t));
    }
    sem_post(&messagesSemaphore_);
  }
  virtual void handleMessage(std::unique_ptr<T> msg) = 0;
  virtual void init() {}
  void stop() {
    running_ = false;
  }
protected:
  virtual void run() {
    // TODO: error handling
    // TODO: do these in main thread
    pthread_mutex_init(&messagesMutex_, NULL);
    sem_init(&messagesSemaphore_, 0, 0);
    init();
    running_ = true;
    while(running_) {
      sem_wait(&messagesSemaphore_);
      std::unique_ptr<T> msg;
      {
        LockMutex lock(&messagesMutex_);
        msg = std::move(messages_.front());
        messages_.pop();
      }
      handleMessage(std::move(msg));
    }
  }
};


