#include <assert.h>
#include "poller.h"
#include "eventLoop.h"

__thread eventLoop *t_loopInThisThread = nullptr;
const int kPollTimeMs = 10000;

//一个线程对应一个eventLoop对象，一个eventLoop含有一个poller分发器
eventLoop::eventLoop()
    :looping_(false),
    quit_(false),
    threadId_(currentThread::tid()),
    poller_(new poller(this)){
    if (t_loopInThisThread){
        /* code */
    }
    else{
        t_loopInThisThread = this;
    }
}

eventLoop::~eventLoop(){
    assert(!looping_);
    t_loopInThisThread = nullptr;
}

void eventLoop::loop(){
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;

    while (!quit_)
    {
        activeChannels.clear();
        poller_->polll(kPollTimeMs, &activeChannels);

        for (auto it = activeChannels.begin(); it != activeChannels.end(); ++it){
            (*it)->handleEvent();
        }
        
    }

    looping_ = false;
}

void eventLoop::quit(){
    quit_ = true;
}

void eventLoop::updateChannel(channel *channel){
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void eventLoop::abortNotInLoopThread(){
    
}