#include <poll.h>
#include <unistd.h>
#include <assert.h>
#include <sys/eventfd.h>
#include "EPollPoller.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logging.h"

//每个EventLoop线程拥有的那个EventLoop*
__thread EventLoop *t_loopInThisThread = 0;

//poller::dispatcher()的等待时间,超过就返回
const int kPollTimeMs = 10000;

int createEventfd(){
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0){
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

//一个线程对应一个eventLoop对象，一个eventLoop含有一个poller分发器
EventLoop::EventLoop()
    :looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    iteration_(0),
    threadId_(CurrentThread::tid()),
    poller_(new EPollPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)){
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread){    //重复创建eventloop
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else{
        t_loopInThisThread = this;
    }
    //当wakeupFd_唤醒poller::dispatcher()时,执行EventLoop::handleRead()函数
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //注册读网络事件,并把它更新到poller::pollfds_上面
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
              << " destructs in thread " << CurrentThread::tid();
    assert(!looping_);
    //取消wakeupChannel_上的所有网络事件,并在poller中移除这个wakeupChannel_
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}


//主要三块:
//1.poller::dispatcher()
//2.Channel::handleEvent()
//3.处理用户任务队列
void EventLoop::loop(){
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";

    while (!quit_)
    {
        //调用poller::dispatcher()获取发生网络事件的套接字
        //利用poller::fillactiveChannels获取套接字的Channel集合
        activeChannels_.clear();
        pollReturnTime_ = poller_->dispatcher(kPollTimeMs, &activeChannels_);
        ++iteration_;
        if (Logger::logLevel() <= Logger::TRACE)
        {
            printActiveChannels();
        }
        //处理所有Channel的网络事件
        eventHandling_ = true;
        for (auto it = activeChannels_.begin(); it != activeChannels_.end(); ++it){
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        eventHandling_ = false;
        //处理用户任务队列
        doPendingFunctors();
    }
    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

//停止EventLoop::loop()
void EventLoop::quit(){
    quit_ = true;
    //只有别的线程让EventLoop线程处理任务时,才可以唤醒
    //不然的话EventLoop线程自己就可以直接退出了
    if(!isInLoopThread()){
        wakeup();
    }
}

//TimerQueue相关,如果用户线程就是EventLoop所在线程,直接执行即可
//否则需要把任务加到任务队列中等待执行
void EventLoop::runInLoop(const Functor& cb){
    if(isInLoopThread()){
        //在EventLoop线程直接执行
        cb();
    }
    else{
        // 如果是其它线程调用runInLoop，则异步地将cb添加到队列
        queueInLoop(std::move(cb));
    }
}

//把任务放进任务队列
void EventLoop::queueInLoop(const Functor& cb){
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    if(!isInLoopThread() || callingPendingFunctors_){
        wakeup();
    }
}

//任务队列的大小
size_t EventLoop::queueSize() const
{
    MutexLockGuard lock(mutex_);
    return pendingFunctors_.size();
}

//定时器相关,设定一个定时器用户执行任务,把定时器加入到timerQueue_中
TimerId EventLoop::runAt(const Timestamp& time, TimerCallback& cb){
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback& cb){
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, TimerCallback& cb){
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

//取消一个定时器
void EventLoop::cancel(TimerId timerid)
{
    return timerQueue_->cancel(timerid);
}

//用wakeupFd_唤醒poller::dispatcher(),使其立即返回
void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if(n != sizeof one){
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

//更新channel,实际上调用了poller::updateChannel,更新poller的pollfds_数组
void EventLoop::updateChannel(Channel *channel){
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

//移除channel,实际上调用了poller::removeChannel,更新poller的pollfds_数组
void EventLoop::removeChannel(Channel* channel){
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(),
                         activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

//断言,当前线程就是EventLoop所在的线程
void EventLoop::assertInLoopThread()
{
    if(!isInLoopThread())
        abortNotInLoopThread();
}
bool EventLoop::isInLoopThread() const{
    return threadId_ == CurrentThread::tid();
}
void EventLoop::abortNotInLoopThread(){
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " <<  CurrentThread::tid();
}

//获取当前线程的那个EventLoop*
EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

//定时器相关的,一旦wakeupFd_发起读网络事件,就执行这个handleRead
void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one)){
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

//处理用户任务队列
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

//打印当前活动的所有channel
void EventLoop::printActiveChannels() const{
    for (const Channel* channel : activeChannels_)
    {
        LOG_TRACE << "{" << channel->reventsToString() << "} ";
    }
}