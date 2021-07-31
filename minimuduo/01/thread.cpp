#include "thread.h"
#include "currentThread.h"
#include <assert.h>

using namespace std;

struct ThreadData{
    typedef thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    pid_t* tid_;

    ThreadData(const ThreadFunc& func, pid_t* tid) :
        func_(func),
        tid_(tid){

        }

    void runInThread(){
        *tid_ = currentThread::tid();
        tid_ = nullptr;
        func_();
    }
};

void* startThread(void *obj){
    ThreadData *data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return nullptr;
}

thread::thread(const ThreadFunc& func)
    :started_(false),
    joined_(false),
    tid_(0),
    pthreadId_(0),
    func_(func){

    }

thread::~thread(){
    if (started_ && !joined_){
        pthread_detach(pthreadId_);
    }
}

void thread::start(){
    assert(!started_);
    started_ = true;

    ThreadData *data = new ThreadData(func_, &tid_);

    if (pthread_create(&pthreadId_, NULL, &startThread, data)){
        started_ = false;
        delete data;
    }
}

int thread::join(){
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}