#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include "nonCopyable.h"
#include <stdint.h>

template <typename T>
class AtomicIntegerT : public nonCopyable{
private:
    volatile T value_;

public:
    AtomicIntegerT()
        :value_(0){

    }
    ~AtomicIntegerT() = default;

    T get(){
        return __sync_val_compare_and_swap(&value_, 0, 0);
    }

    T getAndAdd(T x){
        return __sync_fetch_and_add(&value_, x);
    }

    T addAndGet(T x){
        return getAndAdd(x) + x;
    }

    T incrementAndGet(){
        return addAndGet(1);
    }

    T getAndSet(T newValue){
        return __sync_lock_test_and_set(&value_, newValue);
    }
};

typedef AtomicIntegerT<int32_t> AtomicInt32;
typedef AtomicIntegerT<int64_t> AtomicInt64;

#endif