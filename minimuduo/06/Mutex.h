#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "nonCopyable.h"
#include "CurrentThread.h"
#include <pthread.h>
#include <assert.h>

class MutexLock : public  nonCopyable{
public:
    MutexLock()
        :holder_(0){
        pthread_mutex_init(&mutex_, NULL);
    }
    ~MutexLock(){
        assert(0 == holder_);
        pthread_mutex_destroy(&mutex_);
    }

    void lock(){
        pthread_mutex_lock(&mutex_);
        assignHolder();
    }

    void unlock(){
        unassignHolder();
        pthread_mutex_unlock(&mutex_);
    }

    bool isLockedByThisThread() const{
        return holder_ == CurrentThread::tid();
    }

    void assertLocked() const{
        assert(isLockedByThisThread());
    }

    pthread_mutex_t* getMutex(){
        return &mutex_;
    }

private:
    friend class Condition;

    class UnassignGuard : public nonCopyable{
    public:
        UnassignGuard(MutexLock &owner)
            :owner_(owner){
            owner_.unassignHolder();
        }
        ~UnassignGuard(){
            owner_.assignHolder();
        }
    private:
        MutexLock &owner_;
    };
    
    void unassignHolder(){
        holder_ = 0;
    }

    void assignHolder(){
        holder_ = CurrentThread::tid();
    }

    pid_t holder_;
    pthread_mutex_t mutex_;
};

class MutexLockGuard : public nonCopyable{
public:
    MutexLockGuard(MutexLock &mutex)
        :mutex_(mutex){
        mutex_.lock();
    }
    ~MutexLockGuard(){
        mutex_.unlock();
    }

private:
    MutexLock &mutex_;
};

#endif