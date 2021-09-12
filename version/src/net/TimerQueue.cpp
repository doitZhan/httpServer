#include "TimerQueue.h"
#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"
#include "Logging.h"
#include <functional>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

int createTimerfd();
struct timespec howMuchTimeFromNow(Timestamp when);
void readTimerfd(int timerfd, Timestamp now);
void resetTimerfd(int timerfd, Timestamp expiration);

//loop为timerqueue所属于的那个Eventloop
TimerQueue::TimerQueue(EventLoop *loop)
    :loop_(loop),
    timerfd_(createTimerfd()),//新建timerfd
    timerfdChannel_(loop, timerfd_),
    timers_(),
    callingExpiredTimers_(false){
    //TimerQueue对象成功构造后,timerfd_关联到timerfdChannel_上
    //当timerfd_上读事件发生时,timerfdChannel_回调TimerQueue::handleRead(),
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();    //timerfdChannel_注册读事件,用于接受定时器事件
}

//定时器结束后,禁止timerfdChannel_的所有网络事件,并移除他
TimerQueue::~TimerQueue(){
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    //清空TimerList,由于timers_->second是个指针,应当回收内存
    for (auto it = timers_.begin(); it != timers_.end(); ++it){
        delete it->second;
    }
}

//重设已过期的定时器集合
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now){
    Timestamp nextExpire;

    for(std::vector<Entry>::const_iterator it = expired.begin(); it != expired.end(); ++it){
        if(it->second->repeat()){
            //重新调用addTimer
            it->second->restart(now);
            insert(it->second);
        }
        else{
            delete it->second;
        }
    }

    //如果定时器集合是非空，那么更新下一个超时时刻
    if(!timers_.empty()){
        nextExpire = timers_.begin()->second->expiration();
    }

    if(nextExpire.valid()){
        resetTimerfd(timerfd_, nextExpire);
    }
}

//当timerfd_可读网络事件触发时调用
void TimerQueue::handleRead(){
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    //要读走这个事件,不然会一直触发,因为默认是采用的lt模式
    readTimerfd(timerfd_, now);

    //获取过期的定时器集合
    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();

    //过期的timer开始执行,在chnanel中执行
    for(std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it){
        //timer::run(),执行创建timer时绑定的回调函数
        it->second->run();
    }
    callingExpiredTimers_ = false;
    //重设过期时间,周期性定时器需要重置
    reset(expired, now);
}

//插入一个定时器
bool TimerQueue::insert(Timer* timer){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first){
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool>result = timers_.insert(Entry(when, timer));
        assert(result.second);
        (void) result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
        (void) result;
    }

    assert(timers_.size() == activeTimers_.size());

    return earliestChanged;
}

//从timers_中移除已到期的timer,并通过vector返回这些过期的timer
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    // active timer erase.
    for(std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it){
        ActiveTimer timer(it->second, it->second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(1 == n);
        (void) n;
    }

    assert(timers_.size() == activeTimers_.size());

    return expired;
}

//创建一个定时器,获取其ID,会让eventloop调用addTimerInLoop方法添加一个定时器
TimerId TimerQueue::addTimer(TimerCallback&& cb, Timestamp when, double interval){
    //创建一个新的timer,回调为TimerCallback,在when时触发定时器事件,每隔interval触发一次
    Timer *timer = new Timer(cb, when, interval);
    //在Eventloop中回调 TimerQueue::addTimerInLoop 用于把新建的定时器timer添加到Eventloop中
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

//在Eventloop循环内加入一个定时器
//每当添加一个定时器的时候,timerfd_就被设置为经过timer->expiration()s就会发生
//读网络事件,此时poller::poll()会截取到这个读网络事件,然后交给timerfdChannel_
//去处理,执行TimerQueue::handleRead()函数:获取到所有过期(可执行)的定时器集合,
//去执行这些定时器设置好的回调函数
void TimerQueue::addTimerInLoop(Timer *timer){
    loop_->assertInLoopThread();    //one loop one thread
    bool earliestChanged = insert(timer);

    //只有在计时器为空的时候或者新加入的计时器的最早触发时间小于当前计时器的堆顶的最小值
    //才需要用最近时间去更新
    if (earliestChanged){
        resetTimerfd(timerfd_, timer->expiration());
    }
}

//Eventloop中回调 Timerqueue::cancelInLoop 用于取消一个定时器
void TimerQueue::cancel(TimerId timerId){
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

//关闭Eventloop内的定时器id
void TimerQueue::cancelInLoop(TimerId timerId){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    //找到这个timerId的timer
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if(it != activeTimers_.end()){
        //从timers_中删除这个timer
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(1 == n);
        (void)n;
        delete it->first;
        activeTimers_.erase(it);
    }
    else if(callingExpiredTimers_){
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

//创建一个timerfd,用于产生定时器事件:在poller::poll()获取定时器事件信息
int createTimerfd(){
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if(timerfd < 0){
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

//从现在到when这个时间戳还有多长时间,用timespec表示
struct timespec howMuchTimeFromNow(Timestamp when){
    //剩余微秒数
    int64_t microseconds = when.microSecSinceEpoch() - Timestamp::now().microSecSinceEpoch();
    if(microseconds < 100){
        microseconds = 100;//最小为100微秒
    }

    struct timespec ts;
    //设置秒和毫秒
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecPerSecond) * 1000);
    return ts;
}

//从timerfd这个文件描述符上读取一个uint64_t
void readTimerfd(int timerfd, Timestamp now){
    uint64_t howmany;
    ssize_t n = read(timerfd, &howmany, sizeof howmany);
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if(n != sizeof howmany){
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

//重置timerfd
void resetTimerfd(int timerfd, Timestamp expiration){
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);

    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if(ret){
        LOG_SYSERR << "timerfd_settime()";
    }
}