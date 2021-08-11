#ifndef __TCPCONNECTION_H__
#define __TCPCONNECTION_H__

#include "nonCopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "StringPiece.h"
#include <memory>
#include <string>

class EventLoop;
class Socket;
class Channel;
class Buffer;

class TcpConnection : public nonCopyable, public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(EventLoop *loop, const std::string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb){
        messageCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback& cb){
        closeCallback_ = cb;
    }

    // when output buffer is empty
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){
        writeCompleteCallback_ = cb;
    }
    
    // when the length of output buffer is larger than length user expects. 
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark){
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    void connectEstablished();
    void connectDestroyed();

    EventLoop* getLoop() const{
        return loop_;
    }

    const std::string& name() const{
        return name_;
    }

    const InetAddress& localAddress() const{
        return localAddr_;
    }

    const InetAddress& peerAddress() const{
        return peerAddr_;
    }

    bool connected() const{
        return state_ == KConnected;
    }

    bool disconnected() const{
        return state_ == KDisconnected;
    }

    void send(const void* message, int len);
    void send(const std::string& message);
    void shutdown();
private:
    enum StateE {KConnecting, KConnected, KDisconnecting, KDisconnected};
    void setState(StateE s){
        state_ = s;
    }

    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop *loop_;
    std::string name_;
    StateE state_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    size_t highWaterMark_;
    
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};


#endif