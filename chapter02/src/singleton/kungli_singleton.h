#pragma once

#include <pthread.h>

// kungli's singleton
// lazy loading
// destroy is supported.
//
namespace mymuduo {

class Singleton {
 public:
  static Singleton& GetInstance() {
    pthread_once(&init_once_, &Singleton::Init);
    return *value_;
  }

  static void DestroyInstance() {
    pthread_once(&init_once_, &Singleton::Destroy);
    value_ = nullptr;
  }

  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;

 private:
  Singleton() = default;
  ~Singleton() = default;

  static void Init() {
    value_ = new Singleton();
  }

  static void Destroy() {
    delete value_;
  }

  static Singleton* value_;
  static pthread_once_t init_once_;
  static pthread_once_t destroy_once_;
};

Singleton* Singleton::value_ = nullptr;
pthread_once_t Singleton::init_once_ = PTHREAD_ONCE_INIT;
pthread_once_t Singleton::destroy_once_ = PTHREAD_ONCE_INIT;

}  // namespace mymuduo
