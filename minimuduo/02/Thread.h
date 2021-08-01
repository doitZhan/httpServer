#ifndef __THREAD_H__
#define __THREAD_H__

#include "nonCopyable.h"
#include <pthread.h>
#include <functional>

class Thread : public nonCopyable
{
public:
    typedef std::function<void ()> ThreadFunc;
    Thread(const ThreadFunc&);
    ~Thread();

    void start();
    int join();

    bool started();
    pid_t tid();

private:
    bool started_;
    bool joined_;
    pid_t tid_;
    pthread_t pthreadId_;
    ThreadFunc func_;
};



#endif