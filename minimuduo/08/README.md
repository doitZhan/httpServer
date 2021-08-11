# minimudo Implementation

* version00: <br>
    构建一个简单的one loop per thread框架，启动主线程和子线程的loop。

* version01: <br>
    添加一个channel类，注册回调函数以及事件。
    poller类中维护一个fd-channel映射表。
    eventLoop类中维护一个poller分发器以及活跃事件表。

* version02: <br>
    在传统的IO多路复用系统中，定时操作通常是直接去设置poll()等函数的超时时间，系统超时之后去执行对应的定时回调。但现代Linux系统已经将时间也抽象为了一种事件，并提供了对应的文件描述符fd机制，其能够被poll()等多路复用函数进行监视，因此可以很简洁地融入到我们之前实现的 Reactor机制中。这也是更科学的做法。<br>
    添加Timer、TimerId、TimeQueue类。<br>
    (Reference: [基于timer fd的定时器机制](https://www.jianshu.com/p/02dc5364a173)) <br>

* version03: <br>
    添加EventLoopThread、Mutex、以及Condition一些类。<br>
    EventLoopThread把EventLoop和Thread串联起来，工作流程为：<br>
    1、在主线程（暂且这么称呼）创建EventLoopThread对象。 <br>
    2、主线程调用EventLoopThread.start()，启动EventLoopThread中的线程，这是主线程要等待IO线程创建完成EventLoop对象。 <br>
    3、IO线程调用threadFunc创建EventLoop对象。通知主线程已经创建完成。 <br>
    4、主线程返回创建的EventLoop对象。<br>

* version04: <br>
    添加Acceptor、InetAddress、Socket、SocketsOps、StringPiece类。
    Acceptor：在EventLoop的loop循环中注册监听套接字事件，通过handleRead处理新的连接。

* version05: <br>
    添加TcpConnection和TcpServer类。

* version06: <br>
    添加Buffer类。

* version07: <br>
    添加EventLoopThreadPool类。<br>

* version08: <br>
    添加日志相关的类。<br>