#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "Callbacks.h"
#include "EventLoopThreadPool.h"
#include "Logging.h"
#include <stdio.h>

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option)
    :loop_(loop),
    ipPort_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    threadPool_(new EventLoopThreadPool(loop, nameArg)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    nextConnId_(1){
    //acceptor_接受一个新的连接时,回调TcpServer::newConnection
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

//析构函数,关闭所有TcpConnection时,让TcpConnection回调&TcpConnection::connectDestroyed关闭连接
TcpServer::~TcpServer(){
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for(ConnectionMap::iterator it(connections_.begin()); it != connections_.end(); ++it){
        TcpConnectionPtr conn(it->second);
        it->second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

//设置线程池线程数量
void TcpServer::setThreadNum(int numThreads){
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

//启动TcpServer,实质上启动内部线程池,并让EventLoop回调Acceptor::listen(),完成监听操作
void TcpServer::start(){
    if(started_.getAndSet(1) == 0){
        threadPool_->start(threadInitCallback_);
        assert(!acceptor_->listenning());
        loop_->runInLoop(std::bind(&Acceptor::listen, get_pointer(acceptor_)));
    }
}

//在新连接到达时,Acceptor会先回调Acceptor::handleRead()接受一个连接
//此时连接已经被Acceptor接受完成,然后Acceptor会调用&TcpServer::newConnection()
//调用的目的是把这个连接本身封装成一个TcpConnection,并把它加入到ConnectionMap,
//TcpConenction目的是便于对tcp连接进行管理
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr){
    loop_->assertInLoopThread();
    //在线程池内找到一个EventLoopThread,让其执行新建一个TcpConenction操作
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[32];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName
             << "] from " << peerAddr.toIpPort();

    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    //TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    //新建一个tcp连接,这个操作在线程池内的ioloop线程中执行
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(ioLoop, connName, sockfd, localAddr, peerAddr);

    connections_[connName] = conn;  //更新TcpServer内部连接集合
    //设置TcpConnection的四个回调函数
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    //让EventLoop回调TcpConnection::connectEstablished完成连接建立完成时操作
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

//conn关闭时EventLoop回调TcpServer::removeConnectionInLoop用于关闭tcp连接
void TcpServer::removeConnection(const TcpConnectionPtr& conn){
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn){
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_ << "] - connection " << conn->name();
    //在connections_中移除conn
    size_t n = connections_.erase(conn->name());
    assert(n == 1);
    (void) n;
    EventLoop* ioLoop = conn->getLoop();
    //让EventLoop回调TcpConnection::connectDestroyed完成TcpConnection的销毁
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
