#ifndef __CURRENTTHREAD_H__
#define __CURRENTTHREAD_H__

#include <pthread.h>

namespace CurrentThread{
    extern __thread int t_cachedTid;

    pid_t gettid();
    void cachedTid();

    inline int tid(){
        /* __builtin_expect函数是GCC的一个内建函数(build-in function) */
        // if (t_cachedTid == 0)，值为假的可能性更大
        if (__builtin_expect(t_cachedTid == 0, 0)){
            cachedTid();
        }
        return t_cachedTid;
    }
}


#endif