#include <atomic>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <unistd.h>
#include "mutex.h"

using namespace mymuduo;

MutexLock cout_mtx;
struct Foo {
  int val{0};
};

void ReadFoo(const std::shared_ptr<Foo>& ptr) {
  MutexLockGuard mtx_guard(cout_mtx);
  std::cout << ptr->val << std::endl;
}

void WriteFoo(const std::shared_ptr<Foo>& ptr) {
  ++ptr->val;
  {
    MutexLockGuard mtx_guard(cout_mtx);
    std::cout << "Write is done." << std::endl;
  }
}

namespace version1 {

std::shared_ptr<Foo> global_ptr;
MutexLock ptr_mtx;

void Read() {
  std::shared_ptr<Foo> local_ptr;
  {
    MutexLockGuard mtx_guard(ptr_mtx);
    local_ptr = global_ptr; // copy semantics is not heavy
  }
  ReadFoo(local_ptr);
}

void Write() {
  std::shared_ptr<Foo> new_ptr = std::make_shared<Foo>();
  WriteFoo(new_ptr);
  {
    MutexLockGuard mtx_guard(ptr_mtx);
    global_ptr = new_ptr;
  }
}

}  // namespace version1

namespace version2 {

class FooMgr {
 public:
  FooMgr(const FooMgr&) = delete;
  FooMgr& operator=(const FooMgr&) = delete;

  static FooMgr& GetInstance() {
    static FooMgr foo_mgr;
    return foo_mgr;
  }

  void Read() const {
    auto local_ptr = dbd_[ver_];  // atomic var is not heavy
    ReadFoo(dbd_[ver_]);
  }

  void Write() {
    std::shared_ptr<Foo> new_ptr = std::make_shared<Foo>();
    WriteFoo(new_ptr);
    {
      // switch ab side
      auto new_ver = ++ver_ % kABSide;
      dbd_[new_ver] = new_ptr;
      ver_ = new_ver;
    }
  }

 private:
  FooMgr() {
    for (int i = 0; i < kABSide; ++i) {
      dbd_[i] = std::make_shared<Foo>();
    }
  }
  ~FooMgr() = default;

 private:
  static constexpr int kABSide{2};
  std::shared_ptr<Foo> dbd_[kABSide];
  mutable std::atomic<int> ver_{0};
};

}  // namespace version2

void* ReadRoutine(void* args) {
  for (int i = 0; i < 8; ++i) {
    version2::FooMgr::GetInstance().Read();
    sleep(1);
  }
  return nullptr;
}
void* WriteRoutine(void* args) {
  version2::FooMgr::GetInstance().Write();
  return nullptr;
}

int main() {
  pthread_t tid1, tid2;
  if (pthread_create(&tid1, nullptr, ReadRoutine, nullptr)) {
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&tid2, nullptr, WriteRoutine, nullptr)) {
    exit(EXIT_FAILURE);
  }

  pthread_join(tid1, nullptr);
  pthread_join(tid2, nullptr);

  return 0;
}
