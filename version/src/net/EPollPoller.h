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

    //构造函数,指定EPollPoller所属的那个EventLoop对象
    EPollPoller(EventLoop *loop);
    ~EPollPoller();

    //作为EPollPoller类的核心,EventLoop在loop()中调用dispatcher()函数获得当前活跃的IO事件
    //然后填充EventLoop生成的所有channel到activeChannels中,保存所有活跃的channel信息
    Timestamp dispatcher(int timeoutMs, channelList *activeChannels);

    //添加Channel到channelMap中,并注册/更新/删除Channel对应的fd到epollfd_句柄
    void updateChannel(Channel *channel);
    //从channels_中移除Channel,并删除Channel对应的fd到epollfd_句柄
    void removeChannel(Channel *channel);

    //查找channels_中是否存在channel
    bool hasChannel(Channel* channel) const;

    //保证EventLoop所在的线程为当前线程
    void assertInLoopThread() const{
        ownerLoop_->assertInLoopThread();
    }

private:
    //初始的事件列表大小为16,表示epoll_wait()返回的最多可读或可写事件个数
    static const int kInitEventListSize = 16;
    static const char* operationToString(int op);
    
    //将epoll_wait返回的活跃事件填充到activeChannel
    void fillActiveChannels(int numEvents, channelList *activeChannel) const;
    //注册/更新/删除epollfd_句柄中的fd,封装epoll_ctl
    void update(int operation, Channel* channel);

    typedef std::vector<struct epoll_event> eventList;
    typedef std::map<int, Channel*> channelMap;

    EventLoop *ownerLoop_;      //epoller所属于的那个EventLoop对象
    eventList events_;          //一次epoll_wait所得到的事件集合
    channelMap channels_;       //保存从fd到Channel*的映射.
    int epollfd_;               //epoll的文件描述符,注册事件
};

#endif