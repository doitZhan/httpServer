#ifndef __POLLER_H__
#define __POLLER_H__

#include "nonCopyable.h"
#include "timestamp.h"
#include "eventLoop.h"
#include <map>
#include <vector>
#include "channel.h"
#include <poll.h>

class poller : public nonCopyable
{
public:
    typedef std::vector<channel*> channelList;

    poller(eventLoop *loop);
    ~poller() = default;

    timestamp polll(int timeoutMs, channelList *activeChannels);

    void updateChannel(channel *channel);

    void assertInLoopThread(){
        ownerLoop_->assertInLoopThread();
    }

private:
    void fillActiveChannels(int numEvents, channelList *activeChannel) const;

    typedef std::vector<struct pollfd> pollfdList;
    typedef std::map<int, channel*> channelMap;

    eventLoop *ownerLoop_;
    pollfdList pollfds_;
    channelMap channels_;
};

#endif