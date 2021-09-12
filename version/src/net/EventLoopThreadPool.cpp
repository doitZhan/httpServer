#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

#include <assert.h>
#include <stdio.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *loop, const std::string& nameArg)
    :baseLoop_(loop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0){

}

//创建numThreads_个线程,并且填充threads_集合和loops_集合
void EventLoopThreadPool::start(const ThreadInitCallback& cb){
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;
    //在线程池内创建numThreads_个线程
    for(int i = 0; i < numThreads_; i++){
        char buf[name_.size() + 32];        //线程池内各个线程的名字
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(t);
        loops_.push_back(t->startLoop());
    }

    if(numThreads_ == 0 && cb){     //没有任何线程的话,直接调用callback
        cb(baseLoop_);
    }
}

//获取loops_[next_]的那个EventLoop*
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

//获取第hashCode个EventLoop*
EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode){
    baseLoop_->assertInLoopThread();
    EventLoop* loop = baseLoop_;

    if(!loops_.empty()){
        loop = loops_[hashCode % loops_.size()];
    }

    return loop;
}

//获取所有的EventLoop*
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