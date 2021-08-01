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

class Channel;
class Poller;

class EventLoop : public nonCopyable{
public:
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
private:
    typedef std::vector<Channel *> channelList;

    void abortNotInLoopThread();

    bool looping_;
    bool quit_;
    const pid_t threadId_;
    channelList activeChannels;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
};


#endif