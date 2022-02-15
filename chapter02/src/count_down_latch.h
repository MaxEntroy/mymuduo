#pragma once

#include "condition.h"
#include "mutex.h"

namespace mymuduo {

class CountDownLatch {
 public:
  explicit CountDownLatch(int count) : count_(count), cond_(mtx_) {}

  CountDownLatch(const CountDownLatch&) = delete;
  CountDownLatch& operator=(const CountDownLatch&) = delete;

  void Wait();

  void CountDown();

  int GetCount() const;

 private:
  int count_;
  mutable MutexLock mtx_;
  Condition cond_;
};

}  // namespace mymuduo
