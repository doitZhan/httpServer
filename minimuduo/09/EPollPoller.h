#ifndef __EPOLLPOLLER_H__
#define __EPOLLPOLLER_H__

#include "nonCopyable.h"
#include "Timestamp.h"
#include "EventLoop.h"
#include <map>
#include <vector>

class EPollPoller : public nonCopyable
{
public:
    typedef std::vector<Channel*> channelList;

    EPollPoller(EventLoop *loop);
    ~EPollPoller();

    Timestamp dispatcher(int timeoutMs, channelList *activeChannels);

    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    bool hasChannel(Channel* channel) const;

    void assertInLoopThread() const{
        ownerLoop_->assertInLoopThread();
    }

private:
    static const int kInitEventListSize = 16;
    static const char* operationToString(int op);
    
    void fillActiveChannels(int numEvents, channelList *activeChannel) const;
    void update(int operation, Channel* channel);

    typedef std::vector<struct epoll_event> eventList;
    typedef std::map<int, Channel*> channelMap;

    EventLoop *ownerLoop_;
    eventList events_;
    channelMap channels_;
    int epollfd_;
};

#endif