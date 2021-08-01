#include <poll.h>
#include <unistd.h>
#include <assert.h>

#include "Poller.h"
#include "Channel.h"
#include "EventLoop.h"

__thread EventLoop *t_loopInThisThread = nullptr;
const int kPollTimeMs = 10000;

//一个线程对应一个eventLoop对象，一个eventLoop含有一个poller分发器
EventLoop::EventLoop()
    :looping_(false),
    quit_(false),
    threadId_(CurrentThread::tid()),
    poller_(new Poller(this)),
    timerQueue_(new TimerQueue(this)){
    if (t_loopInThisThread){
        /* code */
    }
    else{
        t_loopInThisThread = this;
    }
}

EventLoop::~EventLoop(){
    assert(!looping_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop(){
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;

    while (!quit_)
    {
        activeChannels.clear();
        poller_->dispatcher(kPollTimeMs, &activeChannels);

        for (auto it = activeChannels.begin(); it != activeChannels.end(); ++it){
            (*it)->handleEvent();
        }
        
    }

    looping_ = false;
}

void EventLoop::quit(){
    quit_ = true;
}

void EventLoop::updateChannel(Channel *channel){
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread(){
    
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb){
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb){
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb){
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}