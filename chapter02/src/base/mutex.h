#pragma once

#include <pthread.h>

#include "common.h"

namespace mymuduo {

class MutexLock {
 public:
  MutexLock() { MCHECK(pthread_mutex_init(&mtx_, nullptr)); };

  MutexLock(const MutexLock&) = delete;
  MutexLock& operator=(const MutexLock&) = delete;

  ~MutexLock() { MCHECK(pthread_mutex_destroy(&mtx_)); }

  void Lock() { MCHECK(pthread_mutex_lock(&mtx_)); }

  void UnLock() { MCHECK(pthread_mutex_unlock(&mtx_)); }

  pthread_mutex_t* GetMutex() { return &mtx_; }

 private:
  pthread_mutex_t mtx_;
};

// Scoped Locking
class MutexLockGuard {
 public:
  explicit MutexLockGuard(MutexLock& mtx_lock) : mtx_lock_(mtx_lock) { mtx_lock_.Lock(); }

  MutexLockGuard(const MutexLockGuard&) = delete;
  MutexLockGuard& operator=(const MutexLockGuard&) = delete;

  ~MutexLockGuard() { mtx_lock_.UnLock(); }

 private:
  MutexLock& mtx_lock_;
};

}  // namespace mymuduo
