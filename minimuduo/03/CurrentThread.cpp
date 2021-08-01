#include "CurrentThread.h"

#include <unistd.h>
#include <sys/syscall.h>

/*
 * 在linux下每一个进程都一个进程id，类型pid_t，可以由getpid（）获取。
 * POSIX线程也有线程id，类型pthread_t，可以由pthread_self（）获取，线程id由线程库维护。
 * 但是各个进程独立，所以会有不同进程中线程号相同的情况。
 * 那么这样就会存在一个问题，我的进程p1中的线程pt1要与进程p2中的线程pt2通信怎么办，进程id不可以，线程id又可能重复，
 * 所以这里会有一个真实的线程id唯一标识，tid。
 * glibc没有实现gettid的函数，所以我们可以通过linux下的系统调用syscall(SYS_gettid)来获得。
*/

namespace CurrentThread{
    __thread int t_cachedTid = 0;

    pid_t gettid(){
        return static_cast<pid_t>(syscall(SYS_gettid));
    }

    void cachedTid(){
        if (t_cachedTid == 0){
            t_cachedTid = gettid();
        }
    }
}