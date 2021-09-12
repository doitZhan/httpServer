#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "nonCopyable.h"

struct tcp_info;
class InetAddress;

class Socket : public nonCopyable{
public:
    //explicit禁止隐式转换
    explicit Socket(int sockfd)
        :sockfd_(sockfd){

    }
    //析构时关闭套接字
    ~Socket();

    //获取sockfd_
    int fd() const{
        return sockfd_;
    }

    //获取TCP信息,利用getsockopt()实现
    bool getTcpInfo(struct tcp_info*) const;
    //获取TCP信息,保存到字符串buf中
    bool getTcpInfoString(char *buf, int len) const;

    //绑定
    void bindAddress(const InetAddress& localaddr);
    //监听
    void listen();
    //接受一个连接
    int accept(InetAddress *peeraddr);
    //关闭写操作
    void shutdownWrite();

    //设置TCP no delay,下面四个都是通过setsockopt()函数设置套接字模式
    void setTcpNoDelay(bool on); //nagle's algorithm
    //地址与端口重用
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    //设置keepalive
    void setKeepAlive(bool on);

private:
    //套接字文件描述符,用const修饰,只能在构造函数初始化列表中初始化,不允许作为左值
    const int sockfd_;
};

#endif