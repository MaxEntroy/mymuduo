#pragma once

#include <pthread.h>
#include "common.h"

namespace mymuduo {

template<typename T>
class ThreadLocal {
 public:
  ThreadLocal() { MCHECK(pthread_key_create(&pkey_, ThreadLocal::dtor)); }

  ~ThreadLocal() { MCHECK(pthread_key_delete(pkey_)); }

  ThreadLocal(const ThreadLocal&) = delete;
  ThreadLocal& operator=(const ThreadLocal&) = delete;

  T& value() {
    T* per_thread_val = static_cast<T*>(pthread_getspecific(pkey_));
    if (!per_thread_val) {
      T* val = new T();
      MCHECK(pthread_setspecific(pkey_, val));
      per_thread_val = val;
    }
    return *per_thread_val;
  }

 private:
  static void dtor(void* x) {
    T* obj = static_cast<T*>(x);
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 0];
    T_must_be_complete_type dummy; (void) dummy;
    delete obj;
  }

 private:
  pthread_key_t pkey_;
};

}  // namespace mymuduo
