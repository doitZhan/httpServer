#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "Atomic.h"
#include "nonCopyable.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include <map>
#include <memory>
#include <string>

class Acceptor;
class EventLoop;
class EventLoopThreadPool;
typedef std::function<void(EventLoop*)> ThreadInitCallback;

class TcpServer : public nonCopyable{
public:
    TcpServer(EventLoop *loop, const InetAddress& listenAddr, const std::string& nameArg);
    ~TcpServer();

    void start();

    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb){
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb){
        writeCompleteCallback_ = cb;
    }
    
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb){
        threadInitCallback_ = cb;
    }
    std::shared_ptr<EventLoopThreadPool> threadPool(){
        return threadPool_;
    }
private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;
    AtomicInt32 started_;
    int nextConnId_;
    ConnectionMap connections_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;
};



#endif