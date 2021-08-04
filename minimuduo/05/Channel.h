#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "nonCopyable.h"
#include <functional>
#include <memory>

class EventLoop;

class Channel : public nonCopyable{
public:
    typedef std::function<void()> EventCallBack;

    Channel(EventLoop *loop, int fd);
    ~Channel() = default;

    void handleEvent();
    void setReadCallback(const EventCallBack& cb){
        readCallback_ = cb;
    }
    void setWriteCallback(const EventCallBack& cb){
        writeCallback_ = cb;
    }
    void setErrorCallback(const EventCallBack& cb){
        errorCallback_ = cb;
    }

    int fd() const{
        return fd_;
    }

    int events() const{
        return events_;
    }

    void setRevents(int revt){
        revents_ = revt;
    }

    bool isNoneEvent()const{
        return events_ == kNoneEvent;
    }

    void enableReading(){
        events_ |= kReadEvent;
        update();
    }

    void enableWriting(){
        events_ |= kWriteEvent;
        update();
    }

    void disableWriting(){
        events_ &= ~kWriteEvent;
        update();
    }

    void disableAll(){
        events_ = kNoneEvent;
        update();
    }

    int index()const{
        return index_;
    }

    void setIndex(int idx){
        index_ = idx;
    }

    EventLoop* ownerLoop()const{
        return loop_;
    }

    void remove();

    // avoid the owner object being destoryed during handleEvent
    void tie(const std::shared_ptr<void>&);
    
private:
    void update();
    void handleEventWithGuard();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;
    bool addToLoop_;

    std::weak_ptr<void> tie_;
    bool tied_;
    bool eventHandling_;

    EventCallBack readCallback_;
    EventCallBack writeCallback_;
    EventCallBack errorCallback_;
    EventCallBack closeCallback_;
};

#endif