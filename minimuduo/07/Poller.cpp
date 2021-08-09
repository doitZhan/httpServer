#include <assert.h>
#include <poll.h>
#include "Channel.h"
#include "Poller.h"

Poller::Poller(EventLoop *loop)
    :ownerLoop_(loop){
}

Timestamp Poller::dispatcher(int timeoutMs, channelList *activeChannels){
    int numEvents = poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    Timestamp now(Timestamp::now());

    if (numEvents > 0){
        fillActiveChannels(numEvents, activeChannels);
    }
    else if (numEvents == 0){

    }
    else{

    }
    
    return now;
}

void Poller::fillActiveChannels(int numEvents, channelList *activeChannel) const{
    for (auto pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; ++pfd){
        if (pfd->revents > 0){
            --numEvents;
            channelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel *channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->setRevents(pfd->revents);
            activeChannel->push_back(channel);
        }
    }
}

void Poller::updateChannel(Channel *channel){
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
            pfd.fd = -channel->fd() -1; ////这样子设置是为了removeChannel优化
        }
    }
}

void Poller::removeChannel(Channel* channel){
    Poller::assertInLoopThread();
    // LOG_TRACE
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());

    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd& pfd = pollfds_[idx];
    (void) pfd;
    // muduo book p287
    assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
    size_t n = channels_.erase(channel->fd());
    assert(1 == n);
    (void) n;
    // channel:poller = n:1, channel:fd = 1:1
    // after removing channel, pollfds_ remove the corresponding fd
    if(static_cast<size_t>(idx) == pollfds_.size() - 1){
        pollfds_.pop_back();
    }
    else{
        int channelAtEnd = pollfds_.back().fd;
        iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
        if(channelAtEnd < 0){
            channelAtEnd = -channelAtEnd - 1;  //还原出来，得到真实的fd，这就是不使用-1的原因
        }
        channels_[channelAtEnd]->setIndex(idx);
        pollfds_.pop_back();
    }
}