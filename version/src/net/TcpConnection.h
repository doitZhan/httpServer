#ifndef __TCPCONNECTION_H__
#define __TCPCONNECTION_H__

#include "nonCopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "StringPiece.h"
#include "AnyType.h"
#include <memory>
#include <string>

class EventLoop;
class Socket;
class Channel;
class Buffer;

class TcpConnection : public nonCopyable, public std::enable_shared_from_this<TcpConnection>{
public:
    //构造函数,初始化成员,设置四个回调函数,Read,Write,Close,ErrorCallback
    TcpConnection(EventLoop *loop, const std::string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    //析构函数,打印一下日志啥也不干
    ~TcpConnection();

    //获取成员信息:EventLoop,name,server/client地址,连接是否建立
    EventLoop* getLoop() const{ return loop_; }
    const std::string& name() const{ return name_; }
    const InetAddress& localAddress() const{ return localAddr_; }
    const InetAddress& peerAddress() const{ return peerAddr_; }
    bool connected() const{ return state_ == KConnected; }
    bool disconnected() const{ return state_ == KDisconnected; }

    //内部利用Socket::getTcpInfo()获取tcp信息
    bool getTcpInfo(struct tcp_info*) const;
    //内部利用Socket::getTcpInfoString()格式化tcp信息
    string getTcpInfoString() const;

    void send(const void* message, int len);
    void send(const std::string& message);
    void send(Buffer* buf);

    //关闭tcp连接,并让EventLoop回调&TcpConnection::shutdownInLoop()
    void shutdown();
    //让EventLoop强制关闭tcp连接,利用EventLoop的定时器队列实现(唤醒epoller::dispatch()来关闭)
    void forceClose();
    //强制在seconds秒后关闭tcp连接
    void forceCloseWithDelay(double seconds);
    //sockets::setTcpNoDelay()
    void setTcpNoDelay(bool on);

    //EventLoop回调TcpConnection::startReadInLoop()
    void startRead();
    //EventLoop回调TcpConnection::stopReadInLoop()
    void stopRead();
    bool isReading() const{ return reading_; }

    void setContext(const Any& context){ context_ = context; }
    const Any& getContext()const{ return context_; }
    Any* getNonConstContext(){ return &context_; }

    //设置四个回调函数
    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb){
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){
        writeCompleteCallback_ = cb;
    }
    // when the length of output buffer is larger than length user expects. 
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark){
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    //获取读/写缓冲区
    Buffer* inputBuffer(){ return &inputBuffer_; }
    Buffer* outputBuffer(){ return &outputBuffer_; }

    //仅作为内部使用
    void setCloseCallback(const CloseCallback& cb){
        closeCallback_ = cb;
    }

    void connectEstablished();
    void connectDestroyed();
   
private:
    enum StateE {
        KConnecting,        //正在连接
        KConnected,         //已连接
        KDisconnecting,     //正在断开
        KDisconnected       //未连接
    };

    void setState(StateE s){
        state_ = s;
    }
    //把tcp连接状态转换成字符串形式
    const char* stateToString() const;

    //处理读/写网络事件,供channel回调
    void handleRead(Timestamp t);
    void handleWrite();
    //处理关闭连接的网络事件,让channel不再关心任何网络事件
    void handleClose();
    //处理错误事件,利用sockets::getSocketError()来实现
    void handleError();

    void sendInLoop(const string& message);
    void sendInLoop(const void* message, size_t len);

    //关闭TcpConenction
    void shutdownInLoop();
    //供EventLoop回调使用,用来强制关闭tcp连接
    void forceCloseInLoop();

    //让channel注册读网络事件
    void startReadInLoop();
    //让channel不再关心读网络事件
    void stopReadInLoop();

    EventLoop *loop_;                                   //所属的EventLoop
    std::string name_;
    StateE state_;                                      //连接状态,有上面四种
    bool reading_;                                      //是否正在读
    std::unique_ptr<Socket> socket_;                    //server Socket*
    std::unique_ptr<Channel> channel_;                  //socket对应的channel
    const InetAddress localAddr_;                       //server地址
    const InetAddress peerAddr_;                        //client地址
    ConnectionCallback connectionCallback_;             //连接成功的回调函数
    MessageCallback messageCallback_;                   //读事件成功的回调函数
    CloseCallback closeCallback_;                       //连接关闭时的回调函数
    WriteCompleteCallback writeCompleteCallback_;       //写事件完成的回调函数
    HighWaterMarkCallback highWaterMarkCallback_;
    size_t highWaterMark_;
    
    Buffer inputBuffer_;        //TCP读缓冲区
    Buffer outputBuffer_;       //TCP写缓冲区

    Any context_;
};


#endif