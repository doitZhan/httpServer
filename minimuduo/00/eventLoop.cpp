#include <poll.h>
#include <assert.h>

#include "eventLoop.h"

__thread eventLoop *t_loopInThisThread = nullptr;

eventLoop::eventLoop()
    :looping_(false),
    threadId_(currentThread::tid()){
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

    poll(nullptr, 0, 5000);

    looping_ = false;
}

void eventLoop::abortNotInLoopThread(){
    
}