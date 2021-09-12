#ifndef __EVENTLOOPTHREAD_H__
#define __EVENTLOOPTHREAD_H__

#include "nonCopyable.h"
#include "Mutex.h"
#include "Thread.h"
#include "Condition.h"

#include <string>

class EventLoop;

template<typename To, typename From>
inline To implicit_cast(From const &f)
{
  return f;
}

class EventLoopThread : public nonCopyable{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;
    //构造函数,负责成员变量的初始化注意callback_设置为cb
    //thread_会被用EventLoopThread::threadFunc()初始化,执行threadFunc()
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const std::string& name = std::string());
    //析构函数,负责停止EventLoop,回收thread_
    ~EventLoopThread();
    //返回一个创建好的EventLoop*,负责启动线程调用threadFunc()
    EventLoop* startLoop();

private:
    //EventLoopThread线程函数,在sratLoop中被调用,
    //负责创建一个EventLoop对象,设置loop_,执行用户传入的回调ThreadInitCallback等
    void threadFunc();

    EventLoop *loop_;                   //线程内部的EventLoop*
    bool exiting_;                      //线程是否退出
    Thread thread_;                     //线程
    MutexLock mutex_;                   //互斥锁
    Condition cond_;                    //条件变量
    ThreadInitCallback callback_;       //线程初始化回调函数
};


#endif