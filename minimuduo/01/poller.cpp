#include "poller.h"
#include <assert.h>

poller::poller(eventLoop *loop)
    :ownerLoop_(loop){
}

timestamp poller::polll(int timeoutMs, channelList *activeChannels){
    int numEvents = poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    timestamp now(timestamp::now());

    if (numEvents > 0){
        fillActiveChannels(numEvents, activeChannels);
    }
    else if (numEvents == 0){

    }
    else{

    }
    
    return now;
}

void poller::fillActiveChannels(int numEvents, channelList *activeChannel) const{
    for (auto pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; ++pfd){
        if (pfd->revents > 0){
            --numEvents;
            channelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            channel *channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->setRevents(pfd->revents);
            activeChannel->push_back(channel);
        }
    }
}

void poller::updateChannel(channel *channel){
    assertInLoopThread();
    if (channel->index() < 0){
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->setIndex(idx);
        channels_[pfd.fd] = channel;
    }
    else{
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);

        int idx = channel->index();
        assert(0 <= (idx) && (idx) < static_cast<int>(pollfds_.size()));
        struct pollfd& pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -1);
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;

        if(channel->isNoneEvent()){
            // ignore the pollfd
            pfd.fd = -1;
        }
    }
}