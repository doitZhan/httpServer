#ifndef __THREAD_H__
#define __THREAD_H__

#include "nonCopyable.h"
#include <pthread.h>
#include <functional>
#include "CountDownLatch.h"

class Thread : public nonCopyable
{
public:
    typedef std::function<void ()> ThreadFunc;
    
    explicit Thread(ThreadFunc&& func, const std::string& name = std::string("Anonymous"));
    ~Thread();

    void start();
    int join();

    bool started() const{return started_;};
    pid_t tid();
    const std::string& name() const{
        return name_;
    }

private:
    bool started_;
    bool joined_;
    pid_t tid_;
    pthread_t pthreadId_;
    ThreadFunc func_;
    CountDownLatch latch_;
    std::string name_;
};



#endif