#ifndef __COUNTDOWNLATCH_H__
#define __COUNTDOWNLATCH_H__

#include "nonCopyable.h"
#include "Mutex.h"
#include "Condition.h"

class CountDownLatch : nonCopyable{
public:
    explicit CountDownLatch(int count);
    ~CountDownLatch() = default;

    void wait();

    void countDown();

    int getCount()const;
private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};



#endif