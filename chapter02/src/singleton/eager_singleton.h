#pragma once

#include <pthread.h>

// eager loading
// no destroy

namespace mymuduo {

class Singleton {
 public:
  Singleton& GetInstance() {
    return *value_;
  }

  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;

 private:
  Singleton() = default;
  ~Singleton() = default;

  static Singleton* Init() {
    return new Singleton();
  }

  static Singleton* value_;
};

Singleton* Singleton::value_ = Singleton::Init();

}  // namespace mymuduo
