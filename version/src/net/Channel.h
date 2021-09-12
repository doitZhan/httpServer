#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "nonCopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>
#include <string>

class EventLoop;

class Channel : public nonCopyable{
public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(Timestamp)> ReadEventCallback;

    //构造函数,根据fd创建一个对应的Channel,仅初始化成员
    Channel(EventLoop *loop, int fd);
    ~Channel();

    //处理网络事件
    void handleEvent(Timestamp receiveTime);

    //设置四个回调函数,read,write,close,error Callback函数,在处理event时被调用
    void setReadCallback(ReadEventCallback&& cb){
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback&& cb){
        writeCallback_ = std::move(cb);
    }
    void setErrorCallback(EventCallback&& cb){
        errorCallback_ = std::move(cb);
    }
    void setCloseCallback(EventCallback&& cb){
        closeCallback_ = std::move(cb);
    }

    //返回Channel所负责IO事件的那个fd_
    int fd() const{
        return fd_;
    }

    //返回当前Channel所注册的网络事件events_
    int events() const{
        return events_;
    }

    //设置网络事件
    void setRevents(int revt){
        revents_ = revt;
    }

    //判断当前Channel是否注册了事件
    bool isNoneEvent()const{
        return events_ == kNoneEvent;
    }

    //在events_上注册读/写事件
    void enableReading(){
        events_ |= kReadEvent;
        update();
    }
    void enableWriting(){
        events_ |= kWriteEvent;
        update();
    }

    //在events_上取消读/写事件
    void disableReading(){
        events_ &= ~kReadEvent;
        update();
    }
    void disableWriting(){
        events_ &= ~kWriteEvent;
        update();
    }

    //取消events_所有事件
    void disableAll(){
        events_ = kNoneEvent;
        update();
    }

    //for poller,当前Channel在poller::pollfds_中的位置
    int index()const{
        return index_;
    }
    void setIndex(int idx){
        index_ = idx;
    }

    //for debug
    std::string reventsToString() const;
    std::string eventsToString() const;

    // avoid the owner object being destoryed during handleEvent
    void tie(const std::shared_ptr<void>&);
    
    //判断events_是否注册了读/写事件
    bool isReading() const{
        return events_ & kReadEvent;
    }
    bool isWriting() const{
        return events_ & kWriteEvent;
    }

    //关闭打印日志
    void doNotLogHup(){
        logHup_ = false;
    }

    //返回当前Channel所在的那个Eventloop
    EventLoop* ownerLoop()const{
        return loop_;
    }

    //让Eventloop移除自身这个Channel
    void remove();

private:
    static std::string eventsToString(int fd, int ev);
    //让本channel所属的那个eventloop回调channel::update()完成channel的更新
    //实际上最终在poller中被更新
    void update();
    //在handleEvent()内部使用的具体的实现
    void handleEventWithGuard(Timestamp receiveTime);

    //这三个静态常量分别表示:无网络事件,读网络事件,写网络事件
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;   //channel所属的那个eventloop
    const int fd_;      //每个channel负责处理一个sockfd上的网络事件
    int events_;        //channel注册(要监听)的网络事件
    int revents_;       //poll/epoll返回的网络事件,具体发生的事件
    int index_;         //这个channel在poller中pollfds_中的序号,默认-1表示不在其中
    bool addToLoop_;    //是否被添加到eventloop中执行
    bool logHup_;       //是否打印日志
    std::weak_ptr<void> tie_;   //负责生存期控制
    bool tied_;
    bool eventHandling_;    //是否正在处理网络事件

    //当发生了读/写/错误网络事件时,下面三个函数会被调用
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};

#endif