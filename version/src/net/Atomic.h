/*
为什么需要原子操作？
最典型的例子：x++
我们知道它有三个步骤，1.从内存中读x的值到寄存器中;2.对寄存器加1;3.再把新值写回x所处的内存地址
假设x的初始值为0，我们使用两个线程对x++，我们期待x的值为3，但实际上可能为2。
原因是有可能多个处理器同时从各自的缓存中读取变量i，分别进行加一操作，然后分别写入系统内存当中。
那么想要保证读改写共享变量的操作是原子的，就必须保证CPU1读改写共享变量的时候，CPU2不能操作缓存了该共享变量内存地址的缓存。

要想安全得到正确的答案：
1.使用互斥锁，但是多个线程访问势必出现锁竞争，锁竞争性能杀手之一，不推荐
2.使用原子性操作，就是不可再分，把三个过程作为一个整体。
*/

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include "nonCopyable.h"
#include <stdint.h>

template <typename T>
class AtomicIntegerT : public nonCopyable{
public:
    AtomicIntegerT()
        :value_(0){

    }
    ~AtomicIntegerT() = default;

    T get(){
        return __sync_val_compare_and_swap(&value_, 0, 0);    //先判定value_是否和oldval相等，如果相等设置value_=newval,返回value_原值，如果不相等直接返回value_原值
    }

    T getAndAdd(T x){
        return __sync_fetch_and_add(&value_, x);    //原子性的让value_=value_+x,返回value_初始值
    }

    T addAndGet(T x){    //返回value_+x的值
        return getAndAdd(x) + x;
    }

    T incrementAndGet(){    //返回value_+1的值
        return addAndGet(1);
    }

    T decrementAndGet(){    //返回value_-1的值
        return addAndGet(-1);
    }

    void add(T x){    //value_+x
        getAndAdd(x);
    }

    void increment(){    //value_+1
        incrementAndGet();
    }

    void decrement(){    //value_-1
        decrementAndGet();
    }

    T getAndSet(T newValue){
        return __sync_lock_test_and_set(&value_, newValue); //将&value_设为newValue并返回value_操作之前的值。
    }

private:
    volatile T value_;  //告诉编译器不要优化代码，每次都要从内存中读取数据（防止读取旧的缓存）
};

typedef AtomicIntegerT<int32_t> AtomicInt32;
typedef AtomicIntegerT<int64_t> AtomicInt64;

#endif