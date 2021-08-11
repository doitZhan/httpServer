#ifndef __TIMER_H__
#define __TIMER_H__

#include "nonCopyable.h"
#include "Timestamp.h"
#include "Atomic.h"
#include "Callbacks.h"

class Timer : public nonCopyable{
public:
    Timer(const TimerCallback& cb, Timestamp when, double interval)
        :callback_(cb),
        expiration_(when),
        interval_(interval),
        repeat_(interval > 0.0),
        sequence_(s_numCreated_.incrementAndGet()){

    }
    ~Timer() = default;

    void run() const{
        callback_();
    }

    Timestamp expiration() const{
        return expiration_;
    }
    bool repeat() const{
        return repeat_;
    }
    int64_t sequence() const{
        return sequence_;
    }
    static int64_t numCreated(){
        return s_numCreated_.get();
    }

    void restart(Timestamp now);
private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static AtomicInt64 s_numCreated_;
};

#endif