/*
    倒计时器类，多线程控制工具类。
CountDownLatch典型用法：
1、某一线程在开始运行前等待n个线程执行完毕
将CountDownLatch的计数器初始化为new CountDownLatch(n)，每当一个任务线程执行完毕，就将计数器减1 countdownLatch.countDown()。
当计数器的值变为0时，在CountDownLatch上wait()的线程就会被唤醒。
一个典型应用场景就是启动一个服务时，主线程需要等待多个组件加载完毕，之后再继续执行。

2、实现多个线程开始执行任务的最大并行性。
注意是并行性，不是并发，强调的是多个线程在某一时刻同时开始执行。类似于赛跑，将多个线程放到起点，等待发令枪响，然后同时开跑。
做法是初始化一个共享的CountDownLatch(1)，将其计算器初始化为1，多个线程在开始执行任务前首先countdownlatch.wait()。
当主线程调用countDown()时，计数器变为0，多个线程同时被唤醒。

CountDownLatch的不足：
CountDownLatch是一次性的，计算器的值只能在构造方法中初始化一次，之后没有任何机制再次对其设置值，当CountDownLatch使用完毕后，它不能再次被使用。

*/

#ifndef __COUNTDOWNLATCH_H__
#define __COUNTDOWNLATCH_H__

#include "nonCopyable.h"
#include "Mutex.h"
#include "Condition.h"

class CountDownLatch : nonCopyable{
public:
    explicit CountDownLatch(int count);
    ~CountDownLatch() = default;

    void wait();

    void countDown();

    int getCount()const;
private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};



#endif