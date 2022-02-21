#include "count_down_latch.h"

namespace mymuduo {

void CountDownLatch::Wait() {
  MutexLockGuard mtx_guard(mtx_);

  while(count_ != 0) {
    cond_.Wait();
  }
}

void CountDownLatch::CountDown() {
  MutexLockGuard mtx_guard(mtx_);

  count_--;
  if (count_ == 0) {
    cond_.NotifyAll();
  }
}

int CountDownLatch::GetCount() const {
  MutexLockGuard mtx_guard(mtx_);
  return count_;
}

}  // namespace mymuduo
