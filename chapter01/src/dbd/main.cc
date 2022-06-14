#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include <atomic>
#include <memory>
#include <vector>

#include "common.h"
#include "mutex.h"

using RoutineType = void*(*)(void *);

constexpr int kReaderNum = 1 << 3;
constexpr int kReadIterNum = 1 << 21; // almost 2 millon
constexpr int kWriteIterNum = 1 << 1;

struct Foo {
  Foo() = default;
  explicit Foo(int new_val) : val(new_val) {}

  int val{0};
};

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    return milliseconds;
}

namespace version1 {

template<typename T>
class DoubleBuffer {
 public:
  DoubleBuffer() = default;

  DoubleBuffer(const DoubleBuffer&) = delete;
  DoubleBuffer& operator=(const DoubleBuffer&) = delete;

  const T* Read() const { return data_ + index_; }

  void Write(const T& new_val) {
    int new_index = !index_;
    data_[new_index] = new_val;

    index_ = new_index;
  }

 private:
  T data_[2];
  std::atomic<int> index_{0};
};

DoubleBuffer<Foo> dbd;

void* ReadRoutine(void* args) {
  for (int i = 0; i < kReadIterNum; ++i) {
    const auto* ptr = dbd.Read();
    //printf("val = %d.\n", ptr->val);
  }
  (void) args;
  return nullptr;
}

void* WriteRoutine(void* args) {
  for (int i = 0; i < kWriteIterNum; ++i) {
    dbd.Write(Foo(i + 1));
  }
  (void) args;
  return nullptr;
}

}  // namespace version1

namespace version2 {

template<typename T>
class DoubleBuffer {
 public:
  DoubleBuffer() = default;

  DoubleBuffer(const DoubleBuffer&) = delete;
  DoubleBuffer& operator=(const DoubleBuffer&) = delete;

  const T* Read() const { return data_ + index_; }

  void Write(const T& new_val) {
    int new_index = !index_;
    data_[new_index] = new_val;

    index_ = new_index;

    // sleep for a while
    sleep(1);
  }

 private:
  T data_[2];
  std::atomic<int> index_{0};
};

DoubleBuffer<Foo> dbd;

void* ReadRoutine(void* args) {
  for (int i = 0; i < kReadIterNum; ++i) {
    const auto* ptr = dbd.Read();
    //printf("val = %d.\n", ptr->val);
  }
  (void) args;
  return nullptr;
}

void* WriteRoutine(void* args) {
  for (int i = 0; i < kWriteIterNum; ++i) {
    dbd.Write(Foo(i + 1));
  }
  (void) args;
  return nullptr;
}

}  // namespace version2

namespace version3 {

template<typename T>
class DoubleBuffer {
 public:
  DoubleBuffer() {
    for (int i = 0 ; i < 2; ++i) {
      data_[i] = std::make_shared<T>();
    }
  }

  DoubleBuffer(const DoubleBuffer&) = delete;
  DoubleBuffer& operator=(const DoubleBuffer&) = delete;

  std::shared_ptr<T> Read() const { return data_[index_]; }

  void Write(const T& new_val) {
    std::shared_ptr<Foo> new_ptr = std::make_shared<Foo>(new_val);
    int new_index = !index_;
    data_[new_index] = new_ptr;

    index_ = new_index;
  }

 private:
  std::shared_ptr<T> data_[2];
  std::atomic<int> index_{0};
};

DoubleBuffer<Foo> dbd;

void* ReadRoutine(void* args) {
  for (int i = 0; i < kReadIterNum; ++i) {
    auto ptr = dbd.Read();
    //printf("val = %d.\n", ptr->val);
  }
  (void) args;
  return nullptr;
}

void* WriteRoutine(void* args) {
  for (int i = 0; i < kWriteIterNum; ++i) {
    dbd.Write(Foo(i + 1));
  }
  (void) args;
  return nullptr;
}

}  // namespace version3

namespace version4 {

template<typename T>
class DoubleBuffer {
 public:
  DoubleBuffer() {
    for (int i = 0 ; i < 2; ++i) {
      data_[i] = std::make_shared<T>();
    }
  }

  DoubleBuffer(const DoubleBuffer&) = delete;
  DoubleBuffer& operator=(const DoubleBuffer&) = delete;

  std::shared_ptr<T> Read() const {
    int cur_index = index_.load(std::memory_order_acquire);
    return data_[cur_index];
  }

  void Write(const T& new_val) {
    std::shared_ptr<Foo> new_ptr = std::make_shared<Foo>(new_val);
    int new_index = !index_.load(std::memory_order_relaxed);
    data_[new_index] = new_ptr;

    index_.store(new_index, std::memory_order_release);
  }

 private:
  std::shared_ptr<T> data_[2];
  std::atomic<int> index_{0};
};

DoubleBuffer<Foo> dbd;

void* ReadRoutine(void* args) {
  for (int i = 0; i < kReadIterNum; ++i) {
    auto ptr = dbd.Read();
    //printf("val = %d.\n", ptr->val);
  }
  (void) args;
  return nullptr;
}

void* WriteRoutine(void* args) {
  for (int i = 0; i < kWriteIterNum; ++i) {
    dbd.Write(Foo(i + 1));
  }
  (void) args;
  return nullptr;
}

}  // namespace version4


namespace version5 {

template<typename T>
class DoubleBuffer {
  class ReaderLock;
  class TLSMgr;
 public:
  class ScopedPtr;
  DoubleBuffer() : tls_mgr_(this) {}

  void Read(ScopedPtr* ptr);
  void Write(const T& new_val);

 private:
  const T* UnsafeRead() const;
  void AddReaderLock(ReaderLock* reader_lock);
  void RemoveReaderLock(ReaderLock* reader_lock);

 private:
  T data_[2];
  std::atomic<int> index_{0};
  mymuduo::MutexLock modify_mtx_;  // Sequence modification
  mymuduo::MutexLock list_mtx_;  // Sequence assess to reader_lock_list
  std::vector<ReaderLock*> reader_lock_list_;
  TLSMgr tls_mgr_;
};

template<typename T>
class DoubleBuffer<T>::ReaderLock {
 public:
  explicit ReaderLock(DoubleBuffer* db) : db_(db) {
    MCHECK(pthread_mutex_init(&mtx_, NULL));
  }

  ReaderLock(const ReaderLock&) = delete;
  ReaderLock& operator=(const ReaderLock&) = delete;

  ~ReaderLock() {
    if (db_) {
      db_->RemoveReaderLock(this);
    }
    MCHECK(pthread_mutex_destroy(&mtx_));
  }

  void BeginRead() { MCHECK(pthread_mutex_lock(&mtx_)); }
  void EndRead() { MCHECK(pthread_mutex_unlock(&mtx_)); }
  void WaitReadDone() { BeginRead(); EndRead(); }

 private:
  DoubleBuffer* db_;
  pthread_mutex_t mtx_;
};

template<typename T>
class DoubleBuffer<T>::TLSMgr {
 public:
  explicit TLSMgr(DoubleBuffer* db) : db_(db) {
    MCHECK(pthread_key_create(&pkey_, &dtor));
  }

  TLSMgr(const TLSMgr&) = delete;
  TLSMgr& operator=(const TLSMgr&) = delete;

  ~TLSMgr() { MCHECK(pthread_key_delete(pkey_)); }

  ReaderLock* TLSValue() {
    ReaderLock* tls_reader_lock = static_cast<ReaderLock*>(pthread_getspecific(pkey_));
    if (tls_reader_lock) {
      return tls_reader_lock;
    }

    std::unique_ptr<ReaderLock> new_tls_reader_lock(new (std::nothrow) ReaderLock(db_));
    if (!new_tls_reader_lock) {
      return nullptr;
    }
    MCHECK(pthread_setspecific(pkey_, new_tls_reader_lock.get()));
    db_->AddReaderLock(new_tls_reader_lock.get());
    return new_tls_reader_lock.release();
  }

 private:
  static void dtor(void* x) {
    ReaderLock* val = static_cast<ReaderLock*>(x);
    delete val;
  }

 private:
  DoubleBuffer* db_;
  pthread_key_t pkey_;
};

template<typename T>
class DoubleBuffer<T>::ScopedPtr {
 public:
  ScopedPtr() = default;

  ScopedPtr(const ScopedPtr&) = delete;
  ScopedPtr& operator=(const ScopedPtr&) = delete;

  ~ScopedPtr() {
    if (reader_lock_) {
      reader_lock_->EndRead();
    }
  }

  const T* get() const { return data_; }
  const T& operator*() const { return *data_; }
  const T* operator->() const { return data_; }

 private:
  friend class DoubleBuffer;
  const T* data_{nullptr};
  ReaderLock* reader_lock_{nullptr};
};

template<typename T>
void DoubleBuffer<T>::Read(typename DoubleBuffer<T>::ScopedPtr* ptr) {
  auto* tls_reader_lock = tls_mgr_.TLSValue();
  if (!tls_reader_lock) {
    return;
  }

  tls_reader_lock->BeginRead();
  ptr->data_ = UnsafeRead();
  ptr->reader_lock_ = tls_reader_lock;
}

template<typename T>
void DoubleBuffer<T>::Write(const T& new_val) {
  mymuduo::MutexLockGuard modify_mtx_guard(modify_mtx_);

  int new_index = !index_.load(std::memory_order_relaxed);
  data_[new_index] = new_val;

  index_.store(new_index, std::memory_order_release);

  mymuduo::MutexLockGuard list_mtx_guard(list_mtx_);
  for (auto&& read_lock : reader_lock_list_) {
    read_lock->WaitReadDone();
  }
}

template<typename T>
const T* DoubleBuffer<T>::UnsafeRead() const {
  return data_ + index_.load(std::memory_order_acquire);
}

template<typename T>
void DoubleBuffer<T>::AddReaderLock(ReaderLock* reader_lock) {
  if (!reader_lock) {
    return;
  }
  mymuduo::MutexLockGuard list_mtx_guard(list_mtx_);
  reader_lock_list_.emplace_back(reader_lock);
}

template<typename T>
void DoubleBuffer<T>::RemoveReaderLock(ReaderLock* reader_lock) {
  if (!reader_lock) {
    return;
  }
  mymuduo::MutexLockGuard list_mtx_guard(list_mtx_);
  for(auto&& a_lock : reader_lock_list_) {
    if (a_lock == reader_lock) {
      a_lock = reader_lock_list_.back();
      reader_lock_list_.pop_back();
      return;
    }
  }
}

DoubleBuffer<Foo> dbd;

void* ReadRoutine(void* args) {
  for (int i = 0; i < kReadIterNum; ++i) {
    DoubleBuffer<Foo>::ScopedPtr reader;
    dbd.Read(&reader);
    //printf("val = %d.\n", reader->val);
  }
  (void) args;
  return nullptr;
}

void* WriteRoutine(void* args) {
  for (int i = 0; i < kWriteIterNum; ++i) {
    dbd.Write(Foo(i + 1));
  }
  (void) args;
  return nullptr;
}

}  // namespace version5

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

  printf("Time elapsed(ms):%lld.\n", end - begin);
}

int main(void) {
  //benchmark(version1::ReadRoutine, version1::WriteRoutine);
  //benchmark(version2::ReadRoutine, version2::WriteRoutine);
  benchmark(version3::ReadRoutine, version3::WriteRoutine);  // almost 610-620ms
  benchmark(version4::ReadRoutine, version4::WriteRoutine);  // almost 600-620ms
  benchmark(version5::ReadRoutine, version5::WriteRoutine);  // almost 80-90ms
  return 0;
}
