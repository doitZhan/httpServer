#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "eventLoop.h"
#include "thread.h"


void threadFunc(){
    printf("threadFunc(): pid:%d, tid:%d\n", getpid(), currentThread::tid());

    eventLoop loop;

    loop.loop();
}

int main(){
    printf("main(): pid:%d, tid:%d\n", getpid(), currentThread::tid());

    eventLoop loop;

    thread thread(threadFunc);
    thread.start();

    loop.loop();
    pthread_exit(NULL);
    return 0;
}