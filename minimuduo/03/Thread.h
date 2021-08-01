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
    
    explicit Thread(const ThreadFunc&);
    ~Thread();

    void start();
    int join();

    bool started() const{return started_;};
    pid_t tid();

private:
    bool started_;
    bool joined_;
    pid_t tid_;
    pthread_t pthreadId_;
    ThreadFunc func_;
    CountDownLatch latch_;
};



#endif