#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

#include "currentThread.h"
#include "nonCopyable.h"
#include <vector>
#include <memory>

class channel;
class poller;

class eventLoop : public nonCopyable{
public:
    eventLoop();
    ~eventLoop();

    void loop();

    void assertInLoopThread(){
        if (!isInLoopThread()){
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const{
        return threadId_ == currentThread::tid();
    }

    void quit();
    void updateChannel(channel *channel);
private:
    typedef std::vector<channel *> channelList;

    void abortNotInLoopThread();

    bool looping_;
    bool quit_;
    const pid_t threadId_;
    channelList activeChannels;
    std::unique_ptr<poller> poller_;
};


#endif