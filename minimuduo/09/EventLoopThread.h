#ifndef __EVENTLOOPTHREAD_H__
#define __EVENTLOOPTHREAD_H__

#include "nonCopyable.h"
#include "Mutex.h"
#include "Thread.h"
#include "Condition.h"

class EventLoop;

class EventLoopThread : public nonCopyable{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;
    // EventLoopThread();
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop *loop_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    ThreadInitCallback callback_;
};


#endif