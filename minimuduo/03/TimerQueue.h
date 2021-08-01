#ifndef __TIMERQUEUE_H__
#define __TIMERQUEUE_H__

#include <vector>
#include <set>

#include "nonCopyable.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "Channel.h"
#include <sys/timerfd.h>

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : nonCopyable{
public:
    TimerQueue(EventLoop *loop);
    ~TimerQueue();

    TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);

private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;

    void addTimerInLoop(Timer *timer);

    void handleRead();

    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer *timer);

    EventLoop *loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList timers_;
};


#endif