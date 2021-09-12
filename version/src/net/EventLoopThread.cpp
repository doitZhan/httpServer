#include "EventLoopThread.h"
#include "EventLoop.h"
#include "CurrentThread.h"

//构造函数,负责初始化数据成员,注意thread_线程函数是threadFunc
EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const string& name)
    :loop_(NULL),
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc, this), name),
    mutex_(),
    cond_(mutex_),
    callback_(cb){

}

//析构函数,EventLoop::quit,Thread::join()
EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if (loop_ != NULL){
        loop_->quit();
        thread_.join();
    }
}

//获取内部EventLoop对象的地址
EventLoop* EventLoopThread::startLoop(){
    assert(!thread_.started());
    thread_.start();    //创建线程调用threadFunc()函数

    {
        //条件变量保证EventLoop创建完成,loop_被赋值
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL){
            cond_.wait();
        }
    }
    
    return loop_;
}

//线程函数体,负责创建EventLoop对象,以及调用EventLoop::loop()
void EventLoopThread::threadFunc(){
    EventLoop loop;
    if (callback_){
        callback_(&loop);
    }

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
    
    MutexLockGuard lock(mutex_);
    loop_ = NULL;
}