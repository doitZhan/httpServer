#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

#include <pthread.h>
#include <memory>
#include <vector>
#include "CurrentThread.h"
#include "nonCopyable.h"
#include "Timestamp.h"
#include "TimerId.h"
#include "Callbacks.h"
#include "TimerQueue.h"
#include "Mutex.h"


class Channel;
//class Poller;
class EPollPoller;

class EventLoop : public nonCopyable{
public:
    typedef std::function<void()> Functor;
    EventLoop();
    ~EventLoop();

    //Eventloop的核心函数,用于不断循环,在其中调用poller::dispatcher()用于获取发生的网络事件
    void loop();
    //停止loop()
    void quit();

    //poller::dispatcher()返回时,得到其返回的事件
    Timestamp pollReturnTime() const{
        return pollReturnTime_;
    }

    int64_t iteration() const{
        return iteration_;
    }

    //TimerQueue相关,将Functor添加到任务队列pendingFunctors_中准备执行
    // run the callback fuction when in the current IO thread, otherwise add into the task queue of the thread.
    void runInLoop(const Functor& cb);
    // add the callback function into the task queue of the current thread.
    void queueInLoop(const Functor& cb);
    size_t queueSize() const;

    //在time时间戳执行cb
    TimerId runAt(const Timestamp& time, TimerCallback& cb);
    //在delay时间戳之后执行cb
    TimerId runAfter(double delay, TimerCallback& cb);
    //每过interval秒执行cb
    TimerId runEvery(double interval, TimerCallback& cb);

    //取消定时器timerid
    void cancel(TimerId timerid);

    //用于在wakeupFd_发起一个读事件,让poller::dispatcher()立即返回,让其处理用户任务
    void wakeup();

    //更新channel,实际上调用了poller::updatechannel,更新poller的pollfds_数组
    void updateChannel(Channel *channel);
    //实质让poller删除pollfds_中的channel相关套接字
    void removeChannel(Channel *channel);
    //判断是否有该channel
    bool hasChannel(Channel *channel);

    //断言,Eventloop所属于的线程ID就是当前线程的ID
    void assertInLoopThread();
    bool isInLoopThread() const;

    bool eventHandling() const{
        return eventHandling_;
    }

    //获得当前线程的那个EventLoop*
    static EventLoop* getEventLoopOfCurrentThread();

private:
    typedef std::vector<Channel *> channelList;

    void abortNotInLoopThread();
    //定时器相关的,一旦wakeupFd_发起读网络事件,就执行这个handleRead
    void handleRead();
    //处理用户任务队列
    void doPendingFunctors();
    //打印当前活动的所有Channel
    void printActiveChannels() const;

    bool looping_;                          //Eventloop是否正在loop
    bool quit_;                             //是否退出loop
    bool eventHandling_;                    //是否正在处理网络事件
    bool callingPendingFunctors_;           //是否正在执行任务队列中的任务
    int64_t iteration_;                     //迭代次数
    const pid_t threadId_;                  //Eventloop所在的那个线程ID,要求one eventloop one thread
    Timestamp pollReturnTime_;
    std::unique_ptr<EPollPoller> poller_;   //用于在loop()中调用poller::dispatcher()
    std::unique_ptr<TimerQueue> timerQueue_;//定时器队列
    int wakeupFd_;                          //用于唤醒poller::dispatcher()的FD
    std::unique_ptr<Channel> wakeupChannel_;//wakeupFd_对应的Channel
    channelList activeChannels_;             //在poller::dispatcher()中返回的activeChannels,也就是每个网络事件对应的channel
    Channel* currentActiveChannel_;          //当前处理到的那个Channel  
    std::vector<Functor> pendingFunctors_;  //用户任务队列
    mutable MutexLock mutex_;               //任务队列的锁
};


#endif