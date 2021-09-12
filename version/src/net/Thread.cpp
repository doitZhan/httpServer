#include "Thread.h"
#include "Logging.h"
#include "CurrentThread.h"
#include <assert.h>
#include <sys/prctl.h>

using namespace std;

AtomicInt32 Thread::numCreated_;

void afterFork(){ //fork之后打扫战场，子进程中执行
    CurrentThread::t_cachedTid = 0;    //当前为0   //1.先清零tid
    CurrentThread::t_threadName = "main";//为main//为什么要赋值为0和main，因为fork可能在主线程中调用，也可能在子线程中调用。fork得到一个新进程，
    CurrentThread::tid();        //2.此处再缓存tid               //新进程只有一个执行序列，只有一个线程
}

class ThreadNameInitializer{ //线程名初始化
public:
    ThreadNameInitializer(){  
        CurrentThread::t_threadName = "main";   //由下面的init全局对象先触发构造，主线程的名称为main
        CurrentThread::tid();                    //获得tid
        pthread_atfork(NULL, NULL, &afterFork);    //如果我们调用了fork函数，调用成功后子进程会调用afterfork()
    }
};

ThreadNameInitializer init;  //全局变量类，这个对象构造先于main函数，当我们的程序引入这个库时，这个全局函数直接构造，我们程序的main()函数还没有执行。

struct ThreadData{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    pid_t* tid_;
    CountDownLatch *latch_;
    std::string name_;
    
    ThreadData(const ThreadFunc& func, pid_t* tid, CountDownLatch *latch, std::string name)
        :func_(func),
        tid_(tid),
        latch_(latch),
        name_(name){

        }

    void runInThread(){
        *tid_ = CurrentThread::tid();
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;
        CurrentThread::t_threadName = name_.empty()? "Anonymous": name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);
        func_();    //运行线程回调函数
        CurrentThread::t_threadName = "finished";   //运行玩的threadname
    }
};

void* startThread(void *obj){
    ThreadData *data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return nullptr;
}

Thread::Thread(ThreadFunc&& func, const std::string& name)
    :started_(false),
    joined_(false),
    tid_(0),
    pthreadId_(0),
    func_(std::move(func)),
    latch_(1),
    name_(name){
        setDefaultName();
    }

Thread::~Thread(){
    if (started_ && !joined_){
        pthread_detach(pthreadId_);
    }
}

void Thread::start(){
    assert(!started_);
    started_ = true;

    ThreadData *data = new ThreadData(func_, &tid_, &latch_, name_);

    if (pthread_create(&pthreadId_, NULL, &startThread, data)){
        started_ = false;
        delete data;
        LOG_SYSFATAL << "Failed in pthread_create";
    }
    else{
        latch_.wait();
        assert(tid_ > 0);
    }
}

int Thread::join(){
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}

void Thread::setDefaultName(){
    int num = numCreated_.incrementAndGet();
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}