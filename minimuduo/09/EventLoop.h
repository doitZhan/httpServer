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

    void loop();

    void assertInLoopThread(){
        if (!isInLoopThread()){
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const{
        return threadId_ == CurrentThread::tid();
    }

    void quit();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    TimerId runAfter(double delay,const TimerCallback& cb);
    TimerId runEvery(double interval, const TimerCallback& cb);

    // run the callback fuction when in the current IO thread, otherwise add into the task queue of the thread.
    void runInLoop(const Functor& cb);
    // add the callback function into the task queue of the current thread.
    void queueInLoop(const Functor& cb);

    void wakeup();
private:
    typedef std::vector<Channel *> channelList;

    void abortNotInLoopThread();
    // eventfd
    void handleRead();
    // todo task in task queue
    void doPendingFunctors();
    
    bool looping_;
    bool quit_;
    const pid_t threadId_;
    channelList activeChannels;
    // Fuctors in doPendingFunctors() may call queueInLoop(functor) again, so queueInLoop() should wake up the IO thread.
    bool callingPendingFunctors_;
    // wakeup fd:wakeup(), handleRead()
    int wakeupFd_;
    bool eventHandling_;
    std::unique_ptr<EPollPoller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    // wakeupFd
    std::unique_ptr<Channel> wakeupChannel_;
    // doPendingFunctors
    std::vector<Functor> pendingFunctors_;

    MutexLock mutex_;
};


#endif