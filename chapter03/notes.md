### Intro

本章总结了一两种常用的线程模型，归纳了进程间通信与线程同步的最佳实践，以期用简单规范的方式开发功能正确、线程安全的多线程程序

### 进程与线程

chenshuo将进程比作人，将独立的address space比作memory，对于分布式系统当中的一些功能，有了拟人化的理解
- 容错：万一有人突然挂了
- 扩容：新人突然加进来
- 负载均衡：把甲的活挪给乙做
- 退休：甲要修复bug，先别给他派活

线程的特点是共享地址空间，从而可以高效地共享数据。一台机器上的多个进程能高效地共享代码段（操作系统可以映射为同样的物理内存），但不能共享数据。
如果多个进程大量共享内存，等于是把多进程程序当成多线程来写，掩耳盗铃。

这里结合我们初代的推荐系统，彼时通过共享内存的方式支持索引，来达到共享数据的目的。一个cpu挂一个进程，固然代码好写也高效，但经常出现死锁，不就是这个问题嘛。所以后来，大家意识到这种设计的不足，才做了服务化。

chenshuo关于多线程的观点如下：
>"多线程"的价值，我认为是为了更好地发挥多核处理器（multi-cores）的效能。在单核时代，多线程没有多大价值.
Alan Cox 说过："A computer is a state machine.Threads are for people who can’t program state machines.
如果只有一块CPU、一个执行单元，那么确实如Alan Cox 所说，按状态机的思路去写程序是最高效的.

### 单线程服务器的常用编程模型

chenshuo在这里只提到了一种模型，就是Reactor。其具体概念如下：

>The reactor design pattern is an event handling pattern for handling service requests delivered concurrently to a service handler by one or more inputs. The service handler then demultiplexes the incoming requests and dispatches them synchronously to the associated request handlers.

这句话给我们如下的信息
- 是什么：一种设计模式(对于常见问题的通用、可复用的解决办法)，基于事件驱动编程范式下的一种通用做法(经过精心设计的fsm)
- 具体使用场景：并发场景下，处理多组io events

根据其使用场景，很容易锁定I/O Multiplexing，同时，又是event handling pattern，很明显就是基于select/poll/epoll fsm精心设计的一种模式。实际当中是通过epoll来实现
简而言之，Reactor是一种基于I/O Multiplexing和事件驱动的设计模式，主要用来(并发)处理多路低速IO设备。

具体到Reactor的实现，主要是使用"non-blocking IO + IO multiplexing",程序的基本结构是一个事
件循环（event loop），以事件驱动（event-driven）和事件回调的方式实现业务逻辑。

```cpp
/*event-loop*/
while(!done) {
/* Read the next incoming event. Usually this is a blocking function. */
    EVENTS event = readEventFromMessageQueue();

    /* Switch the state and the event to execute the right transition. */
    switch (state)
    {
      case ST_RADIO:
        switch (event)
        {
          case EVT_MODE:
            /* Change the state */
            state = ST_CD;
            break;
          case EVT_NEXT:
            /* Increase the station number */
            stationNumber++;
            break;
        }
        break;

      case ST_CD:
        switch (event)
        {
          case EVT_MODE:
            /* Change the state */
            state = ST_RADIO;
            break;
          case EVT_NEXT:
            /* Go to the next track */
            trackNumber++;
            break;
        }
        break;
    }
}

```

### 多线程服务器常用的编程模型

- one thread per connection
- thread pool
- one loop per thread(multi-reactor)
- Leader/Follower

chenshuo这里的建议是使用第三种，即multi-reactor。下面详细说一下这个模式

此种模型下，程序里的每个IO 线程有一个event loop （或者叫Reactor），用于处理读写和定时事件（无论周期性的还是单次的），代码框架跟之前一样。

libev的作者说：
>One loop per thread is usually a good model. Doing this is almost never
wrong, sometimes a better-performance model exists, but it is always a
good start.

这种模式的好处有
- 线程数目固定，可以在程序启动时设置好。不会频繁的创建与销毁
- 可以很方便的在线程之间调配负载。比如io thread和cpu thread个数
- IO 事件发生的线程是固定的，同一个TCP 连接不必考虑事件并发

同时既有计算，又有IO的服务，可以引入线程池来完成计算部分的工作。

<img width="500"  src="img/multi-reactor.png"/>