#ifndef __TIMERQUEUE_H__
#define __TIMERQUEUE_H__

#include <vector>
#include <set>

#include "nonCopyable.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "Channel.h"
#include <sys/timerfd.h>
#include <algorithm>

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : nonCopyable{
public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    //添加定时器到定时器集合中去,供Eventloop使用来封装Eventloop::runAt(),runAfter(),runEvery()
    TimerId addTimer(TimerCallback&& cb, Timestamp when, double interval);

    //取消一个定时器
    void cancel(TimerId timerId);
private:
    typedef std::pair<Timestamp, Timer*> Entry;     //<时间戳,定时器指针>对
    typedef std::set<Entry> TimerList;              //定时器集合
    typedef std::pair<Timer*, int64_t> ActiveTimer; //<定期器指针,int64_t> 活动定时器
    typedef std::set<ActiveTimer> ActiveTimerSet;   //活动定时器集合

    //在Eventloop循环内加入一个定时器
    void addTimerInLoop(Timer *timer);

    //关闭Eventloop内的定时器id
    void cancelInLoop(TimerId timerId);

    //当定时器触发时调用
    void handleRead();

    //获取所有已过期的定时器集合
    std::vector<Entry> getExpired(Timestamp now);
    //重设已过期的定时器集合
    void reset(const std::vector<Entry>& expired, Timestamp now);

    //插入一个定时器
    bool insert(Timer *timer);

    EventLoop *loop_;           //TimeQueue所属的那个Eventloop
    const int timerfd_;         //timerfd_,关联channel注册可读事件
    Channel timerfdChannel_;    //与timerfd_关联,发生可读事件就执行timer::run()
    TimerList timers_;          //定时器的集合
    ActiveTimerSet activeTimers_;   //当前活动定时器的集合
    bool callingExpiredTimers_;     //是否正在调用定时器回调
    ActiveTimerSet cancelingTimers_;    //是否正在停止定时器
};


#endif