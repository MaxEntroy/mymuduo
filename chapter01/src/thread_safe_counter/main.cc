#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class ThreadSafeCounter {
 public:
  ThreadSafeCounter() = default;

  ThreadSafeCounter(const ThreadSafeCounter&) = delete;
  ThreadSafeCounter& operator=(const ThreadSafeCounter&) = delete;

  ThreadSafeCounter(ThreadSafeCounter&&) noexcept = delete;
  ThreadSafeCounter& operator=(ThreadSafeCounter&&) noexcept = delete;

  int Value() const {
    std::scoped_lock _(mtx_);
    return val_;
  }

  int Incr() {
    std::scoped_lock _(mtx_);
    return ++val_;
  }

 private:
  int val_ = 0;
  mutable std::mutex mtx_;
};

void TestThreadSafeCounter() {
  ThreadSafeCounter counter;
  std::vector<std::thread> threads;
  threads.reserve(10);

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&counter]() noexcept {
      for (int i = 0; i < 100; ++i) {
        counter.Incr();
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  std::cout << "counter = " << counter.Value() << std::endl;
}

int main(void) {
  TestThreadSafeCounter();
  return 0;
}
