#ifndef __ACCEPTOR_H__
#define __ACCEPTOR_H__

#include "nonCopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : nonCopyable
{
public:
    typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

    //构造函数负责实现一个TCP服务端时创建套接字->绑定两个个步骤
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport = true);
    //析构函数,负责关闭套接字,取消acceptChannel_上所有的网络事件,表示退出和不再接受处理任务网络事件
    ~Acceptor();

    //设置接受连接时的回调函数,在成功接收一个客户端的连接时调用newConnectionCallback_
    void setNewConnectionCallback(const NewConnectionCallback& cb){
        newConnectionCallback_ = cb;
    }

    bool listenning() const{    //返回监听状态
        return listenning_;
    }

    void listen();  //监听操作

private:
    void handleRead();      //处理服务端accept

    EventLoop *loop_;           //Acceptor所属的EventLoop
    Socket acceptSocket_;       //接受连接的Socket,即服务端的socket
    Channel acceptChannel_;     //acceptSocket_对应的的channel
    NewConnectionCallback newConnectionCallback_;   //新连接建立时所调用的回调函数
    bool listenning_;           //是否正在监听
    int idleFd_;                //占位fd,::open("/dev/null",O_RDONLY|O_CLOEXEC),用于accept时出现的EMFILE错误
};

#endif