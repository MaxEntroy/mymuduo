#include <stdio.h>
#include <string>

#include "mutex.h"
#include "condition.h"

#include "current_thread.h"
#include "thread_local.h"

namespace test_thread_local {

class Foo {
 public:
  Foo() {
    printf("tid=%d, constructing %p\n", ::mymuduo::CurrentThread::Tid(), this);
  }

  ~Foo() {
    printf("tid=%d, destructing %p %s\n", ::mymuduo::CurrentThread::Tid(), this, name_.c_str());
  }

  Foo(const Foo&) = delete;
  Foo& operator=(const Foo&) = delete;

  const std::string& Name() const { return name_; }
  void SetName(const std::string& name) { name_ = name; }

 private:
  std::string name_;
};

mymuduo::ThreadLocal<Foo> test_obj;

void Print() {
  printf("tid=%d, addr=%p, name=%s\n",
         ::mymuduo::CurrentThread::Tid(),
         &test_obj.value(),
         test_obj.value().Name().c_str());
}

void* ThreadRoutine1(void* arg) {
  Print();
  test_obj.value().SetName("thread routine1");
  Print();

  (void) arg;
  return nullptr;
}

void* ThreadRoutine2(void* arg) {
  Print();
  test_obj.value().SetName("thread routine2");
  Print();

  (void) arg;
  return nullptr;
}

}  // namespace test_thread_local

void TEST_thread_local() {
  pthread_t tid1, tid2;
  MCHECK(pthread_create(&tid1, nullptr, test_thread_local::ThreadRoutine1, nullptr));
  MCHECK(pthread_create(&tid2, nullptr, test_thread_local::ThreadRoutine2, nullptr));

  MCHECK(pthread_join(tid1, nullptr));
  MCHECK(pthread_join(tid2, nullptr));
}

int main(void) {
  TEST_thread_local();
  return 0;
}
