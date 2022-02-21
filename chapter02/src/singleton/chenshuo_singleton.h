#pragma once

#include <pthread.h>

// chenshuo's singleton
// lazy loading
// no destroy(not necessary)
//
namespace mymuduo {

class Singleton {
 public:
  Singleton& GetInstance() {
    pthread_once(&init_once_, &Singleton::Init);
    return *value_;
  }

  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;

 private:
  Singleton() = default;
  ~Singleton() = default;

  static void Init() {
    value_ = new Singleton();
  }

  static Singleton* value_;
  static pthread_once_t init_once_;
};

Singleton* Singleton::value_ = nullptr;
pthread_once_t Singleton::init_once_ = PTHREAD_ONCE_INIT;

}  // namespace mymuduo
