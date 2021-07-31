#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include "eventLoop.h"
#include "thread.h"
#include "channel.h"
#include <string.h>

__thread eventLoop *t_loop;

void timeout(){
    printf("Timeout!\n");
    t_loop->quit();
}

void threadFunc(){
    printf("threadFunc(): pid:%d, tid:%d\n", getpid(), currentThread::tid());

    eventLoop loop;
    t_loop = &loop;

    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    channel channel(&loop, timerfd);
    channel.setReadCallback(timeout);
    channel.enableReading();

    struct itimerspec howlong;
    bzero(&howlong, sizeof howlong);
    howlong.it_value.tv_sec = 5;
    timerfd_settime(timerfd, 0, &howlong, NULL);

    loop.loop();
}

int main(){
    printf("main(): pid:%d, tid:%d\n", getpid(), currentThread::tid());

    eventLoop loop;
    t_loop = &loop;

    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    channel channel(&loop, timerfd);
    channel.setReadCallback(timeout);
    channel.enableReading();

    struct itimerspec howlong;
    bzero(&howlong, sizeof howlong);
    howlong.it_value.tv_sec = 5;
    timerfd_settime(timerfd, 0, &howlong, NULL);

    thread thread(threadFunc);
    thread.start();

    loop.loop();
    pthread_exit(NULL);
    return 0;
}