#pragma once

// meyes's singleton
// lazy loading

namespace mymuduo {

class Singleton {
 public:
  Singleton& GetInstance() {
    static Singleton s;
    return s;
  }

  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;

 private:
  Singleton() = default;
  ~Singleton() = default;
};

}  // namespace mymuduo
