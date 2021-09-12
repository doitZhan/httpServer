#ifndef __EVENTLOOPTHREADPOOL_H__
#define __EVENTLOOPTHREADPOOL_H__

#include "nonCopyable.h"
#include <functional>
#include <vector>
#include <string>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : nonCopyable{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;
    EventLoopThreadPool(EventLoop *loop, const std::string& nameArg);
    ~EventLoopThreadPool() = default;

    void setThreadNum(int numThreads){
        numThreads_ = numThreads;
    }

    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    EventLoop* getNextLoop();
    EventLoop* getLoopForHash(size_t hashcode);
    std::vector<EventLoop*> getAllLoops();

    bool started() const{
        return started_;
    }

    const std::string& name() const{
        return name_;
    }

private:
    EventLoop *baseLoop_;           //属于EventLoopThreadPool的EventLoop对象指针baseLoop_
    std::string name_;              //线程池名字
    bool started_;                  //线程池是否启动
    int numThreads_;                //线程池内线程数量
    int next_;
    std::vector<EventLoopThread*> threads_;     //EventLoopThread集合
    std::vector<EventLoop*> loops_;             //各个EventLoopThread拥有的EventLoop指针集合
};



#endif