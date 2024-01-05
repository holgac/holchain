#pragma once
#include <map>
#include <string>
#include <pthread.h>
#include "exception.h"

class Counter
{
  std::map<std::string, int> counters_;
  pthread_mutex_t mutex_;
public:
  Counter() {
    int r = pthread_mutex_init(&mutex_, NULL);
    if (r != 0) {
      THROW("Mutex init failed: %s", StringUtils::errorString(r));
    }
  }
  ~Counter() {
    pthread_mutex_destroy(&mutex_);
  }
  void bump(const std::string& key, int value = 1) {
    auto it = counters_.find(key);
    if (it == counters_.end()) {
      counters_[key] = value;
    } else {
      it->second += value;
    }
  }
  int get(const std::string& key) {
    auto it = counters_.find(key);
    if (it == counters_.end()) {
      return 0;
    } else {
      return it->second;
    }
  }
};
