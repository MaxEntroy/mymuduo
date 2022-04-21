#include <pthread.h>
#include <sys/time.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>

using RoutineType = void*(*)(void *);
using WriteLock = std::unique_lock<std::shared_mutex>;
using ReadLock = std::shared_lock<std::shared_mutex>;

constexpr int kReaderNum = 1 << 3;
constexpr int kReadIterNum = 1 << 21; // almost 2 millon
constexpr int kWriteIterNum = 1 << 1;

struct Foo {
  int val{0};
};

int ReadFoo(const std::shared_ptr<Foo>& ptr) {
  return ptr->val;
}

void WriteFoo(const std::shared_ptr<Foo>& ptr, int new_val) {
  ptr->val = new_val;
}

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    return milliseconds;
}

namespace version1 {

std::shared_ptr<Foo> global_ptr = std::make_shared<Foo>();
std::mutex ptr_mtx;

void Reader() {
  std::shared_ptr<Foo> local_ptr;
  {
    std::lock_guard<std::mutex> mtx_guard(ptr_mtx);
    local_ptr = global_ptr; // copy semantics is not heavy
  }
  ReadFoo(local_ptr);
}

void Writer(int new_val) {
  std::shared_ptr<Foo> new_ptr = std::make_shared<Foo>();
  WriteFoo(new_ptr, new_val);
  {
    std::lock_guard<std::mutex> mtx_guard(ptr_mtx);
    global_ptr = new_ptr;
  }
}

void* ReadRoutine(void* args) {
  for (int i = 0; i < kReadIterNum; ++i) {
    Reader();
  }
  return nullptr;
}

void* WriteRoutine(void* args) {
  for (int i = 0; i < kWriteIterNum; ++i) {
    Writer(i + 1);
  }
  return nullptr;
}

}  // namespace version1

namespace version2 {

std::shared_ptr<Foo> global_ptr = std::make_shared<Foo>();
std::shared_mutex ptr_mtx;

void Reader() {
  std::shared_ptr<Foo> local_ptr;
  {
    ReadLock r_lock_guard(ptr_mtx);
    local_ptr = global_ptr; // copy semantics is not heavy
  }
  ReadFoo(local_ptr);
}

void Writer(int new_val) {
  std::shared_ptr<Foo> new_ptr = std::make_shared<Foo>();
  WriteFoo(new_ptr, new_val);
  {
    WriteLock w_lock_guard(ptr_mtx);
    global_ptr = new_ptr;
  }
}

void* ReadRoutine(void* args) {
  for (int i = 0; i < kReadIterNum; ++i) {
    Reader();
  }
  return nullptr;
}

void* WriteRoutine(void* args) {
  for (int i = 0; i < kWriteIterNum; ++i) {
    Writer(i + 1);
  }
  return nullptr;
}

}  // namespace version2

namespace version3 {

class FooMgr {
 public:
  FooMgr(const FooMgr&) = delete;
  FooMgr& operator=(const FooMgr&) = delete;

  static FooMgr& GetInstance() {
    static FooMgr foo_mgr;
    return foo_mgr;
  }

  void Reader() const {
    auto local_ptr = dbd_[ver_];  // atomic var is not heavy
    ReadFoo(local_ptr);
  }

  void Writer(int new_val) {
    std::shared_ptr<Foo> new_ptr = std::make_shared<Foo>();
    WriteFoo(new_ptr, new_val);
    {
      // switch ab side
      auto new_ver = ++ver_ % kABSide;
      dbd_[new_ver].swap(new_ptr);  // asignment will leads to coredump
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

void* ReadRoutine(void* args) {
  for (int i = 0; i < kReadIterNum; ++i) {
    FooMgr::GetInstance().Reader();
  }
  return nullptr;
}

void* WriteRoutine(void* args) {
  for (int i = 0; i < kWriteIterNum; ++i) {
    FooMgr::GetInstance().Writer(i + 1);
  }
  return nullptr;
}

}  // namespace version3

namespace version4 {

std::shared_ptr<Foo> global_ptr = std::make_shared<Foo>();

void Reader() {
  auto local_ptr = atomic_load(&global_ptr);
  ReadFoo(local_ptr);
}

void Writer(int new_val) {
  std::shared_ptr<Foo> new_ptr = std::make_shared<Foo>();
  WriteFoo(new_ptr, new_val);
  std::atomic_exchange(&global_ptr, new_ptr);
}

void* ReadRoutine(void* args) {
  for (int i = 0; i < kReadIterNum; ++i) {
    Reader();
  }
  return nullptr;
}

void* WriteRoutine(void* args) {
  for (int i = 0; i < kWriteIterNum; ++i) {
    Writer(i + 1);
  }
  return nullptr;
}

}  // namespace version4

void benchmark(RoutineType read_routine, RoutineType write_routine) {
  pthread_t tid_readers[kReaderNum];
  pthread_t tid_writer;

  auto begin = current_timestamp();
  for (int i = 0; i < kReaderNum; ++i) {
    if (pthread_create(tid_readers + i, nullptr, read_routine, nullptr)) {
      exit(EXIT_FAILURE);
    }
  }
  if (pthread_create(&tid_writer, nullptr, write_routine, nullptr)) {
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < kReaderNum; ++i) {
    pthread_join(tid_readers[i], nullptr);
  }
  pthread_join(tid_writer, nullptr);
  auto end = current_timestamp();

  std::cout << "Time elapsed(ms):" << end - begin << std::endl;
}

int main(void) {
  benchmark(version1::ReadRoutine, version1::WriteRoutine);  // almost 1300ms
  benchmark(version2::ReadRoutine, version2::WriteRoutine);  // almost 1600ms
  benchmark(version3::ReadRoutine, version3::WriteRoutine);  // almost 600ms
  benchmark(version4::ReadRoutine, version4::WriteRoutine);  // almost 1700ms
  return 0;
}
