#include "EventLoopThread.h"
#include "EventLoop.h"
#include "CurrentThread.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
    :loop_(NULL),
    thread_(std::bind(&EventLoopThread::threadFunc, this)),
    mutex_(),
    cond_(mutex_),
    callback_(cb){

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
    printf("threadFunc(): pid:%d, tid:%d\n", CurrentThread::getpid(), CurrentThread::tid());
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
    loop_ = NULL;
}