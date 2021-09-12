#ifndef __POLLER_H__
#define __POLLER_H__

#include "nonCopyable.h"
#include "Timestamp.h"
#include "EventLoop.h"
#include <map>
#include <vector>

class Poller : public nonCopyable
{
public:
    typedef std::vector<Channel*> channelList;

    Poller(EventLoop *loop);
    ~Poller() = default;

    Timestamp dispatcher(int timeoutMs, channelList *activeChannels);

    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    void assertInLoopThread(){
        ownerLoop_->assertInLoopThread();
    }

private:
    void fillActiveChannels(int numEvents, channelList *activeChannel) const;

    typedef std::vector<struct pollfd> pollfdList;
    typedef std::map<int, Channel*> channelMap;

    EventLoop *ownerLoop_;
    pollfdList pollfds_;
    channelMap channels_;
};

#endif