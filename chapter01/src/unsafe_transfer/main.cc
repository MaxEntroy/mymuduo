#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

// No data race, but race condition exists.
// There is no any synchronization mechanism to protect critical region.
bool unsafe_transfer(std::atomic<int>& src, std::atomic<int>& dst, int money) {
  if (money < src) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    src -= money;
    dst += money;
    return true;
  } else {
    return false;
  }
}

bool safe_transfer(std::atomic<int>& src, std::atomic<int>& dst, std::mutex& mtx, int money) {
  std::scoped_lock lock(mtx);
  if (money < src) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    src -= money;
    dst += money;
    return true;
  } else {
    return false;
  }
}

void unsafe_transfer_test() {
  std::atomic<int> my_account{100};
  std::atomic<int> your_account{0};

  std::thread t1(
      [&my_account, &your_account]() noexcept { unsafe_transfer(my_account, your_account, 60); });
  std::thread t2(
      [&my_account, &your_account]() noexcept { unsafe_transfer(my_account, your_account, 60); });

  t1.join();
  t2.join();

  std::cout << "my_account:" << my_account << ",your_account:" << your_account << std::endl;
}

void safe_transfer_test() {
  std::atomic<int> my_account{100};
  std::atomic<int> your_account{0};
  std::mutex mtx;

  std::thread t1(
      [&my_account, &your_account, &mtx]() noexcept { safe_transfer(my_account, your_account, mtx, 60); });
  std::thread t2(
      [&my_account, &your_account, &mtx]() noexcept { safe_transfer(my_account, your_account, mtx, 60); });

  t1.join();
  t2.join();
  std::cout << "my_account:" << my_account << ",your_account:" << your_account << std::endl;
}

int main(void) {
  unsafe_transfer_test();
  safe_transfer_test();
  return 0;
}
