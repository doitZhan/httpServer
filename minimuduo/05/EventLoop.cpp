#include <poll.h>
#include <unistd.h>
#include <assert.h>
#include <sys/eventfd.h>
#include "Poller.h"
#include "Channel.h"
#include "EventLoop.h"

__thread EventLoop *t_loopInThisThread = nullptr;
const int kPollTimeMs = 10000;
int createEventfd();
//一个线程对应一个eventLoop对象，一个eventLoop含有一个poller分发器
EventLoop::EventLoop()
    :looping_(false),
    quit_(false),
    threadId_(CurrentThread::tid()),
    poller_(new Poller(this)),
    timerQueue_(new TimerQueue(this)),
    callingPendingFunctors_(false),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)){
    if (t_loopInThisThread){
        /* code */
    }
    else{
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    assert(!looping_);

    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    close(wakeupFd_);

    t_loopInThisThread = NULL;
}

void EventLoop::loop(){
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_)
    {
        activeChannels.clear();
        poller_->dispatcher(kPollTimeMs, &activeChannels);

        for (auto it = activeChannels.begin(); it != activeChannels.end(); ++it){
            (*it)->handleEvent();
        }
        doPendingFunctors();
    }

    looping_ = false;
}

void EventLoop::quit(){
    quit_ = true;
    if(!isInLoopThread()){
        wakeup();
    }
}

void EventLoop::updateChannel(Channel *channel){
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread(){
    
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb){
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb){
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb){
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

int createEventfd(){
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0){
        // LOG_SYSERR
        abort();
    }
    return evtfd;
}

void EventLoop::runInLoop(const Functor& cb){
    if(isInLoopThread()){
        cb();
    }
    else{
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor& cb){
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }

    if(!isInLoopThread() || callingPendingFunctors_){
        wakeup();
    }
}

void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if(n != sizeof one){
        // LOG_ERROR;
    }
}

void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one)){
        // LOG_ERROR
    }
}

void EventLoop::doPendingFunctors(){
    // narrow the critical area
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(size_t i = 0; i < functors.size(); i++){
        functors[i]();
    }

    callingPendingFunctors_ = false;
}