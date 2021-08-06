#include "EventLoopThread.h"
#include "EventLoop.h"


EventLoopThread::EventLoopThread()
    :loop_(NULL),
    thread_(std::bind(&EventLoopThread::threadFunc, this)),
    mutex_(),
    cond_(mutex_),
    callback_(NULL){

}

EventLoopThread::~EventLoopThread(){
    if (loop_ != NULL){
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop(){
    assert(!thread_.started());
    thread_.start();

    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL){
            cond_.wait();
        }
    }
    
    return loop_;
}

void EventLoopThread::threadFunc(){
    EventLoop loop;
    printf("I am here 1\n");
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        printf("I am here 2\n");
        cond_.notify();
    }

    loop.loop();
    loop_ = NULL;
}