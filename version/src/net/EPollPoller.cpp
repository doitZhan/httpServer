#include "EPollPoller.h"
#include "Channel.h"
#include "Logging.h"

#include <assert.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <poll.h>

// for Channel, default index is -1
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

//构造函数，初始化EPollPoller所属的EventLoop，创建epollfd_
EPollPoller::EPollPoller(EventLoop* loop)
        :ownerLoop_(loop),
        events_(EPollPoller::kInitEventListSize),   //初始化eventList大小
        epollfd_(::epoll_create1(EPOLL_CLOEXEC))  //创建epollfd_
{
    if(epollfd_ < 0){
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }
}

//析构函数，关闭epollfd_
EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

//EPollPoll中的事件分发器
Timestamp EPollPoller::dispatcher(int timeoutMs, channelList* activeChannels){
    LOG_TRACE << "fd total count " << channels_.size();
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0){      //>0表示返回活跃事件个数
        LOG_TRACE << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
        if(static_cast<size_t>(numEvents) == events_.size()){
            events_.resize(events_.size() * 2);     ////如果返回的事件数目等于当前事件数组大小，就扩容events_.size()空间*2
        }
    }
    else if(numEvents == 0){    //=0表示超时返回
        LOG_TRACE << "nothing happened";
    }
    else{   //<0表示error，要查看errno返回的具体错误是什么
        // if the error is not caused by some signal interrupting the system call
        if(savedErrno != EINTR){
            errno = savedErrno;
            LOG_SYSERR << "EPollPoller::dispatcher()";
        }
    }
    return now; //返回时间戳
}

//将活跃fd对应的channel放到activeChannels中
void EPollPoller::fillActiveChannels(int numEvents, channelList* activeChannels) const{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for(int i = 0; i < numEvents; i++){
        //epoll_wait会原封不动返回ptr指向的结构体
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        int fd = channel->fd();  //拿到其中的文件描述符
        channelMap::const_iterator it = channels_.find(fd);  //ChannelMap以文件描述符为key
        assert(it != channels_.end());
        assert(it->second == channel);

        channel->setRevents(events_[i].events);     //将活跃的fd事件类型赋值给Channel类中revents_元素
        activeChannels->push_back(channel);         //添加进活跃事件合集activeChannels
    }
}

//添加Channel到channelMap中,并注册/更新/删除Channel对应的fd到epollfd_句柄
void EPollPoller::updateChannel(Channel* channel){
    assertInLoopThread();
    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events() << " index = " << index;
    if(index == kNew || index == kDeleted){     //一个新的channel，添加到epollfd_句柄
        int fd = channel->fd();
        if(index == kNew){
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else{
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else{   //一个已存在的channel
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if(channel->isNoneEvent()){     //当前channel无网络事件，从epoll_句柄中删除
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        }
        else{
            update(EPOLL_CTL_MOD, channel);     //在epoll_句柄中更新channel
        }
    }
}

//从channels_中移除Channel,并删除Channel对应的fd到epollfd_句柄
void EPollPoller::removeChannel(Channel* channel){
    assertInLoopThread();
    int fd = channel->fd();

    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(1 == n);

    if(index == kAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew);
}

//注册/更新/删除epollfd_句柄中的fd,封装epoll_ctl
void EPollPoller::update(int operation, Channel* channel){
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;   //ptr指针存放fd对应的channel地址
    int fd = channel->fd();

    LOG_TRACE << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd << " event = {" << channel->eventsToString() << "}";
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd = " << fd;
        }
        else{
            LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd = " << fd;
        }
    }
}

// 是否拥有该channel
bool EPollPoller::hasChannel(Channel* channel) const{
    assertInLoopThread();
    // 获取fd对应的通道
    auto it = channels_.find(channel->fd());
    return (it != channels_.end() && it->second == channel);
}

//EPOLL_CTL_*操作转string
const char* EPollPoller::operationToString(int op){
    switch (op){
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            assert(false && "ERROR op");
            return "Unknown Operation";
    }
}

