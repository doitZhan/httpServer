#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "nonCopyable.h"
#include <functional>

class eventLoop;

class channel : public nonCopyable{
public:
    typedef std::function<void()> EventCallBack;

    channel(eventLoop *loop, int fd);
    ~channel() = default;

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

    eventLoop* ownerLoop()const{
        return loop_;
    }
private:
    void update();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    eventLoop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;

    EventCallBack readCallback_;
    EventCallBack writeCallback_;
    EventCallBack errorCallback_;
};

#endif