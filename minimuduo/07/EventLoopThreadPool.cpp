#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

#include <assert.h>
#include <stdio.h>

template<typename To, typename From>
inline To implicit_cast(From const &f){
    return f;
}

EventLoopThreadPool::EventLoopThreadPool(EventLoop *loop, const std::string& nameArg)
    :baseLoop_(loop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0){

}

void EventLoopThreadPool::start(const ThreadInitCallback& cb){
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_ = true;

    for(int i = 0; i < numThreads_; i++){
        EventLoopThread* t = new EventLoopThread(cb);
        threads_.push_back(t);
        loops_.push_back(t->startLoop());
    }

    if(numThreads_ == 0 && cb){
        cb(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop(){
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop = baseLoop_;

    if(!loops_.empty()){
        loop  = loops_[next_];
        next_++;

        if(implicit_cast<size_t>(next_) >= loops_.size()){
            next_ = 0;
        }
    }
    return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode){
    baseLoop_->assertInLoopThread();
    EventLoop* loop = baseLoop_;

    if(!loops_.empty()){
        loop = loops_[hashCode % loops_.size()];
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops(){
    baseLoop_->assertInLoopThread();
    assert(started_);
    if(loops_.empty()){
        return std::vector<EventLoop*>(1, baseLoop_);
    }
    else{
        return loops_;
    }
}