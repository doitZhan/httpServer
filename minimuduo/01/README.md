# minimudo Implementation

* version00: <br>
    构建一个简单的one loop per thread框架，启动主线程和子线程的loop。

* version01: <br>
    添加一个channel类，注册回调函数以及事件。
    poller类中维护一个fd-channel映射表。
    eventLoop类中维护一个poller分发器以及活跃事件表。