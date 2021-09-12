#include "Channel.h"
#include "EventLoop.h"
#include "Logging.h"

#include <poll.h>
#include <assert.h>
#include <sstream>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN;
const int Channel::kWriteEvent = POLLOUT;

//构造函数,根据fd创建一个对应的Channel,仅初始化成员
Channel::Channel(EventLoop* loop, int fd)
    :loop_(loop),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1),
    addToLoop_(false),
    logHup_(true),
    tied_(false),
    eventHandling_(false){

}

//保证这个Channel析构时,Eventloop不再持有这个Channel
Channel::~Channel(){
    assert(!eventHandling_);
    assert(!addToLoop_);
    if(loop_->isInLoopThread())
        assert(!loop_->hasChannel(this));
}

//让本Channel所属于的那个Eventloop完成Channel的更新
void Channel::update(){
    addToLoop_ = true;
    loop_->updateChannel(this);
}

//让Eventloop删除这个Channel
void Channel::remove(){
    assert(isNoneEvent());
    addToLoop_ = false;
    loop_->removeChannel(this);
}

//此时Eventloop::loop()中poll函数返回,说明有网络事件发生了,
//内部利用handleEventWithGuard()实现对各个网络事件的处理
void Channel::handleEvent(Timestamp receiveTime){
    std::shared_ptr<void> guard;
    if(tied_){
        // weak_ptr --> shared_ptr
        guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }
    else{
        handleEventWithGuard(receiveTime);
    }
}

//根据不同的网络事件调用不同的回调处理
void Channel::handleEventWithGuard(Timestamp receiveTime){
    eventHandling_ = true;
    //处理关闭事件
    if((revents_ & POLLHUP) && !(revents_ & POLLIN)){
        if (logHup_){
            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
        }
        if(closeCallback_){
            closeCallback_();
        }
    }

    if(revents_ & POLLNVAL){
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
    }

    //处理错误事件
    if(revents_ & (POLLERR | POLLNVAL)){
        if(errorCallback_){
            errorCallback_();
        }
    }

    //处理读网络事件
    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)){
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }

    //处理写网络事件
    if(revents_ & (POLLOUT)){
        if(writeCallback_){
            writeCallback_();
        }
    }
    eventHandling_ = false;
}

void Channel::tie(const std::shared_ptr<void>& obj){
    tie_ = obj;
    tied_ = true;
}

//for debug
string Channel::reventsToString() const
{
    return eventsToString(fd_, revents_);
}
string Channel::eventsToString() const
{
    return eventsToString(fd_, events_);
}

//把网络事件转化成字符串格式
string Channel::eventsToString(int fd, int ev)
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN)
      oss << "IN ";
    if (ev & POLLPRI)
      oss << "PRI ";
    if (ev & POLLOUT)
      oss << "OUT ";
    if (ev & POLLHUP)
      oss << "HUP ";
    if (ev & POLLRDHUP)
      oss << "RDHUP ";
    if (ev & POLLERR)
      oss << "ERR ";
    if (ev & POLLNVAL)
      oss << "NVAL ";

    return oss.str();
}