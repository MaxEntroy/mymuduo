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

### 进程间通信只用TCP

常见的IPC如下：
- pipe
- shared memory
- posix mq
- singal
- socket

关于IPC的选择，chenshuo的观点如下：
>进程间通信我首选Sockets（主要指TCP，我没有用过UDP，也不考虑Unixdomain 协议），其最大的好处在于：
可以跨主机，具有伸缩性。反正都是多进程了，如果一台机器的处理能力不够，很自然地就能用多台机器来处理。把进程分散到同一
局域网的多台机器上，程序改改host:port 配置就能继续用。相反，前面列出的其他IPC 都不能跨机器10，这就限制了scalability。

TCP socket具有如下优点：
- 全双工(对比pipe)
- 系统自动回收
- tcp port由一个进程独占，防止程序重复启动
- 两个进程通过TCP 通信，如果一个崩溃了，操作系统会关闭连接，另一个进程
几乎立刻就能感知，可以快速failover。应用层只能依赖心跳
- 可记录，可重现

当然，TCP socket也存在一些缺点：
- 有marshal/unmarshal的开销
- 共享内存效率最高

chenshuo这里也建议，即使单机，也没有必要为了一点性能的提升，而使用共享内存。按照我自己的经验来看，可不是这样，比如推荐的请求，通常网络包非常大，网络开销的节省通常会带来显著的性能收益。
共享内存的问题在于，代码不好写，容易死锁。并且只能是单机，如果使用，意味着不同的服务有耦合。

关于长短连接的选择，chenshuo建议只使用长连接，原因如下：
- 一是容易定位分布式系统中的服务之间的依赖关系
  - server side: ```netstat -tpna | grep :port```找出client port
  - client side: ```lsof -i:port```找出客户端进程
- 二是通过接收和发送队列的长度也较容易定位网络或程序故障
  - ```netstat -tn```

### 多线程服务器的适用场合

这里先给出chenshuo整体的观点：
- 分布式系统的软件设计和功能划分一般应该以"进程"为单位
- 从宏观上看，一个分布式系统是由运行在多台机器上的多个进程组成的，进程之间采用TCP 长连接通信。
- 本章讨论分布式系统中单个服务进程的设计方法。我提倡用多线程，并不是说把整个系统放到一个进程里实现，而是
指功能划分之后，在实现每一类服务进程时，在必要时可以借助多线程来提高性能
- 对于整个分布式系统，要做到能scale out，即享受增加机器带来的好处

开发服务端程序的一个基本任务是处理并发连接，现在服务端网络编程处理并发连接主要有两种方式
- 当"线程"很廉价时
  - 一台机器上可以创建远高于CPU 数目的"线程"。
  - 这时一个线程只处理一个TCP 连接（甚至半个），通常使用阻塞IO(至少看起来如此)
  - 例如，Python gevent、Go goroutine、Erlang actor。这里的"线程"由语言的runtime 自行调度，与操作系统线程不是一回事
- 当线程很宝贵时
  - 一台机器上只能创建与CPU 数目相当的线程
  - 这时一个线程要处理多个TCP 连接上的IO，通常使用非阻塞IO 和IO multiplexing
  - libevent、muduo、Netty。这是原生线程，能被操作系统的任务调度器看见

关于线程的理解，这里我多说一点。首先，不得不说的是，chenshuo这本书写于2012年，并且这是一本偏向于应用的书籍(对比apue/unp这种手册)，放在2022年的今天，重读起来发现任然不过时，并且其所讲的核心知识点，任然是广大linux cpp开发者所必备的技能。其次，我们来说下线程这个东西。

"廉价的线程"用目前的术语来讲，说的是协程(coroutine/fiber这一类)。但其实，这个概念具备一定的迷惑性。因为看起来它好像和进程，线程均为kernel提供。但其实不是，chenshuo讲的也很清楚，这个东西
kernel感知不到，完全是语言层面的产物。

对于这个概念的理解，我倾向于用folly当中关于fiber的描述来理解：
>Fibers (or coroutines) are lightweight application threads. Multiple fibers can be running on top of a single system thread. Unlike system threads, all the context switching between fibers is happening explicitly. Because of this every such context switch is very fast (~200 million of fiber context switches can be made per second on a single CPU core).

很明显，folly将coroutines/fibers理解为lightweight application threads，有别于system threads，前者由语言层面进行调度。这个东西真正的好处是，虽然还是异步框架(本质是异步阻塞)，但是
代码写起来像是同步代码。这里主要是为了优化之前的异步框架，即异步非阻塞，这种通常需要写回调，代码不好写，容器出错。

我们可以看下2022年的当下，目前主流的rpc开发框架所支持的方式，baidu的brpc，tx的trpc采用了异步阻塞即协程的方式，具体支持了bthread/fiber这样的组件。搜狗的workflow则还是保持了之前异步回调的方式。