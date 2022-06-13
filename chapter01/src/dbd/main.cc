#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include <atomic>
#include <memory>

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

  void Write(const T new_val) {
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

  void Write(const T new_val) {
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

namespace version5 {

template<typename T>
class DoubleBuffer {

};

}  // namespace version5

int main(void) {
  benchmark(version1::ReadRoutine, version1::WriteRoutine);
  benchmark(version2::ReadRoutine, version2::WriteRoutine);
  benchmark(version3::ReadRoutine, version3::WriteRoutine);
  benchmark(version4::ReadRoutine, version4::WriteRoutine);
  return 0;
}
