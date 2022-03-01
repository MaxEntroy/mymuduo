## Intro

编写线程安全的类不是难事，用同步原语（synchronization primitives）保护内
部状态即可。但是对象的生与死不能由对象自身拥有的mutex（互斥器）来保护。如
何避免对象析构时可能存在的race condition（竞态条件）是C++ 多线程编程面临的
基本问题，可以借助shared_ptr和weak_ptr完美解决。

## 对象的创建很简单

一个线程安全的class 应当满足以下三个条件
- 无论操作系统如何调度这些线程， 无论这些线程的执行顺序如何交织
（interleaving）
- 多个线程同时访问时，其表现出正确的行为
- 调用端代码无须额外的同步或其他协调动作

对象构造要做到线程安全，唯一的要求是在**构造期间不要泄露this指针**，即
- 不要在构造函数中注册任何回调
- 也不要在构造函数中把this 传给跨线程的对象
- 即便在构造函数的最后一行也不行

```cpp
// 不要这么做（Don't do this.）
class Foo : public Observer //
{
 public:
  Foo(Observable＊ s)
  {
    s->register_(this); // 错误，非线程安全
  }
  virtual void update();
};
```

对象构造的正确方法

```cpp
// 要这么做（Do this.）
class Foo : public Observer
{
 public:
  Foo();
  virtual void update();

  // 另外定义一个函数，在构造之后执行回调函数的注册工作
  void observe(Observable＊ s)
  {
    s->register_(this);
  }
};

Foo* pFoo = new Foo;
Observable* s = getSubject();
pFoo->observe(s); // 二段式构造，或者直接写s->register_(pFoo);
```

二段式构造——即构造函数+initialize()——有时会是好办法
- 这虽然不符合C++ 教条，但是多线程下别无选择
- 构造函数不必主动抛异常，调用方靠initialize() 的返回值来判断对象是否构造成
功，这能简化错误处理

即使构造函数的最后一行也不要泄露this，因为Foo 有可能是个基类，基类先于
派生类构造，执行完Foo::Foo() 的最后一行代码还会继续执行派生类的构造函数，
这时most-derived class 的对象还处于构造中，仍然不安全

## 销毁太难

当一个对象能被多个线程同时看到时，那么对象的销毁时机就
会变得模糊不清，可能出现多种竞态条件(race condition)
- 在即将析构一个对象时，从何而知此刻是否有别的线程正在执行该对象的成员
函数？
- 如何保证在执行成员函数期间，对象不会在另一个线程被析构？
- 在调用某个对象的成员函数之前，如何得知这个对象还活着？它的析构函数会
不会碰巧执行到一半

由于dtor会销毁对象内部的mutex，导致后者无法起到保护内部状态的作用

```cpp
Foo::~Foo()
{
  MutexLockGuard lock(mutex_);
  // free internal state (1)
}

void Foo::update()
{ 
  MutexLockGuard lock(mutex_); // (2)
  // make use of internal state
}

// thread A
delete x;
x = NULL; // helpless

// thread B
if (x) {
x->update();
}
```

考虑如下调度顺序:
1. thread A先执行，拿到锁之后，调度
2. thread B后执行，条件判断成功(此时还未真正析构)，进入条件执行，调度
3. thread A接着执行，析构，指针置NULL
4. thread B接着执行，无效地址访问，coredump

## 原始指针有何不妥

指向对象的原始指针（raw pointer）是坏的，尤其当暴露给别的线程时。因为**一个动态创建的对象是否还活着，光看指针是看不出来的**
通过raw pointer实现的observer不是线程安全的，根本原因就在于此


<img width="500"  src="img/dangling-pointer.png"/>