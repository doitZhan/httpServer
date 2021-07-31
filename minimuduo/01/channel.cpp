#include "channel.h"
#include <poll.h>
#include "eventLoop.h"

const int channel::kNoneEvent = 0;
const int channel::kReadEvent = POLLIN;
const int channel::kWriteEvent = POLLOUT;

channel::channel(eventLoop* loop, int fd)
    :loop_(loop),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1){
}

void channel::update(){
    loop_->updateChannel(this);
}

void channel::handleEvent(){
    if (revents_ &  POLLNVAL){

    }

    if (revents_ & (POLLERR | POLLNVAL)){
        if (errorCallback_){
            errorCallback_();
        }
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)){
        if (readCallback_){
            readCallback_();
        }
    }

    if (revents_ & (POLLOUT)){
        if (writeCallback_){
            writeCallback_();
        }
    }
}