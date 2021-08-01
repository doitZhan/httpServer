#include "EventLoop.h"
#include "CurrentThread.h"
#include "Timestamp.h"


#include <functional>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "Thread.h"

__thread int cnt = 0;
__thread EventLoop *t_loop;

void printTid()
{
  printf("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  printf("now %s\n", Timestamp::now().toString().c_str());
}

void print(const char* msg)
{
  printf("msg %s %s\n", Timestamp::now().toString().c_str(), msg);
  if (++cnt == 15)
  {
    t_loop->quit();
  }
}

void threadFunc(){
    printf("threadFunc(): pid:%d, tid:%d\n", getpid(), CurrentThread::tid());

    EventLoop loop;
    t_loop = &loop;

    loop.runAfter(1, std::bind(print, "thread once1"));
    loop.runAfter(2, std::bind(print, "thread once2"));
    loop.runAfter(3, std::bind(print, "thread once3"));
    loop.runEvery(2.5, std::bind(print, "thread every2.5"));
    loop.runEvery(3.5, std::bind(print, "thread every3.5"));

    loop.loop();
    print("thread loop exits");
}

int main()
{
  printTid();
  EventLoop loop;
  t_loop = &loop;

  loop.runAfter(1, std::bind(print, "main once1"));
  loop.runAfter(1.5, std::bind(print, "main once1.5"));
  loop.runAfter(2.5, std::bind(print, "main once2.5"));
  loop.runAfter(3.5, std::bind(print, "main once3.5"));
  loop.runEvery(2, std::bind(print, "main every2"));
  loop.runEvery(3, std::bind(print, "main every3"));

  Thread thread(threadFunc);
  thread.start();

  loop.loop();
  print("main loop exits");
  pthread_exit(NULL);
  return 0;
}
