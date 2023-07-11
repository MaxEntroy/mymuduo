### Intro

并发编程有两种基本模型，分别是
- messsage passing
- shared memory

前者更常见，也更容易保证程序的正确性，但我们仍然需要了解底层的shared memory 模型下的同步原语，以备不时之需。

**线程同步的四项原则**
- 1.首要原则是尽量最低限度地共享对象，减少需要同步的场合。一个对象能不暴
露给别的线程就不要暴露；如果要暴露，优先考虑immutable 对象；实在不行
才暴露可修改的对象，并用同步措施来充分保护它。
- 2.其次是使用高级的并发编程构件，如TaskQueue、Producer-Consumer Queue、
CountDownLatch 等等。
- 3.最后不得已必须使用底层同步原语（primitives）时，**只用**的互斥器和条
件变量，**慎用**读写锁，**不用**信号量。
- 4.除了使用atomic 整数之外，不自己编写lock-free 代码，也不要用“内核级”
同步原语。不凭空猜测“哪种做法性能会更好”，比如spin lock vs. mutex。

### Mutex

mutex的使用原则如下
- 1.用RAII 手法封装mutex 的创建、销毁、加锁、解锁这四个操作
- 2.只用非递归的mutex（即不可重入的mutex）
- 3.使用Scoped Locking Pattern
- 4.在每次构造Guard 对象的时候，思考一路上（调用栈上）已经持有的锁，防止
因加锁顺序不同而导致死锁（deadlock）

#### Using non-recursive mutex

mutex 分为递归（recursive）和非递归（non-recursive）两种, 它们唯一区别在于：同一个线程可以重复
对recursive mutex 加锁，但是不能重复对non-recursive mutex 加锁。

chenshuo对于非递归锁的考虑如下：
>在同一个线程里多次对non-recursive mutex 加锁会立刻导致死锁，我认为这是它的优
点，能帮助我们思考代码对锁的期求，并且及早（在编码阶段）发现问题。

毫无疑问recursive mutex 使用起来要方便一些，因为不用考虑一个线程会自
己把自己给锁死了。正因为它方便，recursive mutex 可能会隐藏代码里的一些问题。典型情况是你
以为拿到一个锁就能修改对象了，没想到外层代码已经拿到了锁，正在修改（或读
取）同一个对象呢

```cpp
MutexLock mutex;
std::vector<Foo> foos;

void post(const Foo& f)
{
  MutexLockGuard lock(mutex);
  foos.push_back(f);
}

void traverse()
{
  MutexLockGuard lock(mutex);
  for (std::vector<Foo>::const_iterator it = foos.begin();
    it != foos.end(); ++it)
  {
    it->doit();
  }
}
```

将来有一天，Foo::doit() 间接调用了post()，那么会很有戏剧性的结果：
1. mutex 是非递归的，于是死锁了。
2. mutex 是递归的，由于push_back() 可能（但不总是）导致vector 迭代器失效，
程序偶尔会crash。

这时候就能体现non-recursive 的优越性：把程序的逻辑错误暴露出来。死锁比
较容易debug.

#### DeadLock Avoidance

如果坚持只使用Scoped Locking，那么在出现死锁的时候很容易定位

```cpp
class Inventory
{
 public:
  void add(Request* req)
  {
    muduo::MutexLockGuard lock(mutex_);
    requests_.insert(req);
  }
  void remove(Request* req) // __attribute__ ((noinline))
  {
    muduo::MutexLockGuard lock(mutex_);
    requests_.erase(req);
  }
  void printAll() const
  {
    muduo::MutexLockGuard lock(mutex_);
    sleep(1); // 为了容易复现死锁，这里用了延时
    for (std::set<Request*>::const_iterator it = requests_.begin(); it != requests_.end(); ++it)
    {
      (*it)->print();
    }
    printf("Inventory::printAll() unlocked\n");
  }
 private:
  mutable muduo::MutexLock mutex_;
  std::set<Request*> requests_;
};
Inventory g_inventory; // 为了简单起见，这里使用了全局对象。

class Request
{
  public:
  void process() // __attribute__ ((noinline))
  {
  muduo::MutexLockGuard lock(mutex_);
  g_inventory.add(this);
  // ...
}

 ~Request() __attribute__ ((noinline))
 {
   muduo::MutexLockGuard lock(mutex_);
   sleep(1); // 为了容易复现死锁，这里用了延时
   g_inventory.remove(this);
 }

 void print() const __attribute__ ((noinline))
 {
   muduo::MutexLockGuard lock(mutex_);
 // ...
 }

 private:
 mutable muduo::MutexLock mutex_;
 };

void threadFunc()
{
  Request* req = new Request;
  req->process();
  delete req;
}
int main()
{
  muduo::Thread thread(threadFunc);
  thread.start();
  usleep(500 * 1000); // 为了让另一个线程等在前面第14 行的sleep() 上。
  g_inventory.printAll();
  thread.join();
}
```
通过打印线程函数调用栈(thread apply all bt)，很容易看出来是死锁了。**因为一个程序中的线程一般只会等在 condition variable 上，或者等在epoll_wait** 
当然，我觉得上面代码死锁的原因主要在于，Inventory是一个线程安全的类，但是client代码还是使用了额外的同步机制，导致死锁。

### Condition Variable

互斥器（mutex）是加锁原语，用来排他性地访问共享数据，它不是等待原语。在使用mutex 的时候，我们一般都会期望加锁不要阻塞，总是能立刻拿到锁。然后
尽快访问数据，用完之后尽快解锁，这样才能不影响并发性和性。

条件变量只有一种正确使用的方式，几乎不可能用错。对于wait 端
- 1.必须与mutex 一起使用，该布尔表达式的读写需受此mutex 保护。
- 2.在mutex 已上锁的时候才能调用wait()。
- 3.把判断布尔条件和wait() 放到while 循环中。

对于signal/broadcast 端：
- 1.在signal 之前一般要修改布尔表达式。
- 2.修改布尔表达式通常要用mutex 保护（至少用作full memory barrier）。
- 3.不一定要在mutex 已上锁的情况下调用signal （理论上/apue并没有这么做）。
- 4.注意区分signal 与broadcast： “broadcast 通常用于表明状态变化，signal 通常用于表示资源可用。（broadcast should generally be used to indicate state change rather than resource availability)”

这里我补充下最佳实践
- 条件变量的语义更近似同步。
- 暂时归纳在producer-consumer模型的工具当中。(countdown_latch对应forks-and-join, mutex对应mutual exclusion，cond对应producer-consumer)
- 一个cond关联一个条件判断(bool表达式)，bool表达式的判断变量有几个，就有几个cond
- wait端的条件判断放在while-loop中，signal端的条件判断放在if judgement中即可

- cond vs count_down_latch
  - 如果是非计数的条件同步，使用cond
  - 如果是计数相关的条件同步，使用count_down_latch
    - count_down_latch封装了cond，并且使用了基于计数判断的条件表达式。所以，它只能用于和计数相关的条件同步

```cpp
// bounded_buffer.h
#pragma once

#include <semaphore.h>
#include <pthread.h>
#include <vector>

class BoundedBuffer {
 public:
  explicit BoundedBuffer(int size);

  BoundedBuffer(const BoundedBuffer&) = delete;
  BoundedBuffer& operator=(const BoundedBuffer&) = delete;

  ~BoundedBuffer();

  void produce(int item);
  int consume();

 private:
  int size_ = 0;
  std::vector<int> buf_;

  int front_ = 0;  // buf_[(front)%size_] is first item
  int rear_ = 0;   // buf_[(rear_-1)%size_] is last item

  pthread_mutex_t mtx_;    // protects assess to buf
  pthread_cond_t not_empty_;  // counts available slots
  pthread_cond_t not_full_;   // counts available items
};

// bounded_buffer.cc
#include "bounded_buffer.h"

#include <assert.h>

BoundedBuffer::BoundedBuffer(int size) : size_(size) {
  buf_.resize(size_);

  assert(pthread_mutex_init(&mtx_, nullptr) == 0);
  assert(pthread_cond_init(&not_empty_, nullptr) == 0);
  assert(pthread_cond_init(&not_full_, nullptr) == 0);
}

BoundedBuffer::~BoundedBuffer() {
  assert(pthread_mutex_destroy(&mtx_) == 0);
  assert(pthread_cond_destroy(&not_empty_) == 0);
  assert(pthread_cond_destroy(&not_full_) == 0);
}

void BoundedBuffer::produce(int item) {
  pthread_mutex_lock(&mtx_);
  while((rear_ + 1) % size_ == front_) {
    pthread_cond_wait(&not_full_, &mtx_);
  }
  buf_[rear_++%size_] = item;
  pthread_mutex_unlock(&mtx_);
  pthread_cond_signal(&not_empty_);
}

int BoundedBuffer::consume() {
  int item = 0;
  pthread_mutex_lock(&mtx_);
  while (front_ == rear_) {
    pthread_cond_wait(&not_empty_, &mtx_);
  }
  item = buf_[front_++%size_];
  pthread_mutex_unlock(&mtx_);
  pthread_cond_signal(&not_full_);
  return item;
}
```

关于```pthread_cond_wait```还有一些细节需要讨论

Q: 为什么需要和锁配合使用？<br>
A: 避免丢失唤醒。即先wait，后signal。还会丢失。这个答案是确定的。参考文献的第一篇篇文章讲的很清楚

Q: 释放锁和进入等待队列为什么原子操作？<br>
A：如果确实是先释放锁，再进入等待队列。那原因是一样的，如果没有原子操作，会导致唤醒丢失。但是如果先加入等待队列，再释放锁。不用，因为此时有锁保护，signal线程拿不到锁，无法实际唤醒。

### CountDownLatch

POSIX API的barries语义
>Barriers are a synchronization mechanism that can be used to coordinate multiple
threads working in parallel. A barrier allows each thread to wait until all cooperating
threads have reached the same point, and then continue executing from there. 

chenshuo这里讲它的目的在于，这个东西可以通过mutex and cond实现，并且确实非常实用

```cpp
class CountDownLatch : boost::noncopyable
{
 public:
  explicit CountDownLatch(int count); // 倒数几次
  void wait(); // 等待计数值变为0
  void countDown(); // 计数减一
 private:
  mutable MutexLock mutex_;
  Condition condition_;
  int count_;
};

void CountDownLatch::wait()
{
  MutexLockGuard lock(mutex_);
  while (count_ > 0)
    condition_.wait();
}
void CountDownLatch::countDown()
{
  MutexLockGuard lock(mutex_);
  --count_;
  if (count_ == 0)
    condition_.notifyAll();
}
```

### Readers-Writer lock/Semaphore

不建议用。
互斥器和条件变量构成了多线程编程的全部必备同步原语，用它们即可完成任何
多线程同步任务。所以信号量是不必要的。

对于读写锁，chenshuo给出的结论我没有实际体会，但是从工作的实践来看，一般都会用double-buffer.

### Singleton

meyes's singleton(local static var)解决了线程安全的问题, 但是eager class static var同样可接解决线程安全问题，为什么还建议这么做?
- cpp只能保证同一个文件中，声明的static变量的初始化顺序与其变量声明的顺序一致,不同文件则无法保证
- 不同的单例之间可能有依赖，如果采用eager class static var可能导致依赖单例未初始化

### Sleep

生产代码中线程的等待可分为两种：
- 一种是等待资源可用（要么等在select/poll/epoll_wait 上，要么等在条件变量上43
- 一种是等着进入临界区（等在mutex上）以便读写共享数据。后一种等待通常极短，否则程序性能和伸缩性就会有问题。

如果多线程的安全性和效率要靠代码主动调用sleep 来保证，这显然是设计出了问题。

### Summary

- 线程同步的四项原则，尽量用高层同步设施（线程池、队列、倒计时）；
- 使用普通互斥器和条件变量完成剩余的同步任务，采用RAII 惯用手法（idiom）
和Scoped Locking。

### References

[calling-pthread-cond-signal-without-locking-mutex](https://stackoverflow.com/questions/4544234/calling-pthread-cond-signal-without-locking-mutex )<br>
[pthread_cond_wait为什么需要传递mutex参数](https://www.zhihu.com/question/24116967)<br>
[thread_cond_timedwait](https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_timedwait.html)<br>
