#pragma once

#include <pthread.h>

#include "common.h"
#include "mutex.h"

namespace mymuduo {

class Condition {
 public:
  explicit Condition(MutexLock& mutex_lock) : mutex_lock_(mutex_lock) {
    MCHECK(pthread_cond_init(&cond_, nullptr));
  }

  Condition(const Condition&) = delete;
  Condition& operator=(const Condition&) = delete;

  ~Condition() { MCHECK(pthread_cond_destroy(&cond_)); }

  void Wait() { MCHECK(pthread_cond_wait(&cond_, mutex_lock_.GetMutex())); }

  void Notify() { MCHECK(pthread_cond_signal(&cond_)); };
  void NotifyAll() { MCHECK(pthread_cond_broadcast(&cond_)); };

 private:
  MutexLock& mutex_lock_;
  pthread_cond_t cond_;
};

}  // namespace mymuduo
