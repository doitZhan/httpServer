#ifndef __THREAD_H__
#define __THREAD_H__

#include "nonCopyable.h"
#include <pthread.h>
#include <functional>
#include "CountDownLatch.h"
#include "Atomic.h"
#include <memory>

class Thread : public nonCopyable
{
public:
    typedef std::function<void ()> ThreadFunc; //std::function可以取代函数指针的作用，因为它可以延迟函数的执行，特别适合作为回调函数使用。
    
    //右值引用，在对象返回的时候不会拷贝构造临时对象，而是和临时对象交换，提高了效率
    explicit Thread(ThreadFunc&& func, const std::string& name = std::string("Anonymous"));
    ~Thread();

    void start();
    int join();

    bool started() const{return started_;};
    pid_t tid(){
        return tid_;
    }
    const std::string& name() const{
        return name_;
    }

    static int numCreated(){ 
        return numCreated_.get(); 
    }

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    pid_t tid_;
    pthread_t pthreadId_;
    ThreadFunc func_;
    CountDownLatch latch_;
    std::string name_;
    static AtomicInt32 numCreated_;
};



#endif