#ifndef __TIMERID_H__
#define __TIMERID_H__

#include <stdint.h>

class Timer;

class TimerId{
private:
    Timer *timer_;
    int64_t sequence_;

public:
    TimerId()
        :timer_(nullptr),
        sequence_(0){

    }

    TimerId(Timer *timer, int64_t seq)
        :timer_(timer),
        sequence_(seq){

    }

    ~TimerId() = default;

    friend class TimerQueue;
};

#endif