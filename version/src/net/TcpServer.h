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
    enum Option{
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop, const InetAddress& listenAddr, const std::string& nameArg, Option option = kNoReusePort);
    ~TcpServer();

    //获得ipPort,name,EventLoop*
    const string& ipPort() const { return ipPort_; }
    const string& name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }

    /// Set the number of threads for handling input.
    ///
    /// Always accepts new connection in loop's thread.
    /// Must be called before @c start
    /// @param numThreads
    /// - 0 means all I/O in loop's thread, no thread will created.
    ///   this is the default value.
    /// - 1 means all I/O in another thread.
    /// - N means a thread pool with N threads, new connections
    ///   are assigned on a round-robin basis.
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb){
        threadInitCallback_ = cb;
    }
    std::shared_ptr<EventLoopThreadPool> threadPool(){
        return threadPool_;
    }

    //Starts the server if it's not listening.
    void start();

    //设置连接建立时,读消息时,写完成时的回调函数
    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb){
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){
        writeCompleteCallback_ = cb;
    }


private:
    //新建一个连接,Accpetor默认的接受连接时的回调函数
    void newConnection(int sockfd, const InetAddress& peerAddr);
    //移除一个连接
    void removeConnection(const TcpConnectionPtr& conn);
    //loop内部移除一个连接
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
    //每个TcpConnection都有一个名字
    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_;                                   //TcpServer所在的EventLoop
    const string ipPort_;                               //TcpServer的ipPort
    const string name_;
    std::unique_ptr<Acceptor> acceptor_;                //Acceptor智能指针,用于创建套接字,绑定,监听和接收一个连接
    std::shared_ptr<EventLoopThreadPool> threadPool_;   //EventLoop线程池智能指针
    ConnectionCallback connectionCallback_;             //建立连接时的回调函数
    MessageCallback messageCallback_;                   //读消息时的回调函数
    WriteCompleteCallback writeCompleteCallback_;       //写事件完成时的回调函数
    ThreadInitCallback threadInitCallback_;             //线程初始化回调
    AtomicInt32 started_;                               //TcpServer是否启动
    int nextConnId_;                                    //下一个连接的client的fd
    ConnectionMap connections_;                         //名字到TcpConnection的映射
};



#endif