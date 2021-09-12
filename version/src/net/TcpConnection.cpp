#include "TcpConnection.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include "Timestamp.h"
#include "SocketsOps.h"
#include "Buffer.h"
#include "Logging.h"

#include <unistd.h>
#include <string.h>

template<typename To, typename From>
inline To implicit_cast(From const& f){
    return f;
}

void defaultConnectionCallback(const TcpConnectionPtr& conn){
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void defaultMessageCallback(const TcpConnectionPtr&, Buffer *buf, Timestamp)
{
    buf->retrieveAll();
}

//构造函数,初始化成员,设置四个回调函数,Read,Write,Close,ErrorCallback
TcpConnection::TcpConnection(EventLoop* loop, const string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
    :loop_(loop),
    name_(nameArg),
    state_(KConnecting),
    reading_(true),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64 * 1024 * 1024){
        // do not use make_shared() in ctor
        channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
        channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
        channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
        channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
        LOG_DEBUG << "TcpConection::ctor[" << name_ << "] at " << this << " fd= " << sockfd;
        socket_->setKeepAlive(true);
}

//析构函数,打印一下日志啥也不干
TcpConnection::~TcpConnection(){
    LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
              << " fd=" << channel_->fd()
              << " state=" << stateToString();
    assert(state_ == KDisconnected);
}

//内部利用Socket::getTcpInfo()获取tcp信息
bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
{
    return socket_->getTcpInfo(tcpi);
}

//内部利用Socket::getTcpInfoString()格式化tcp信息
string TcpConnection::getTcpInfoString() const
{
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}

//发送数据,内部让EventLoop回调TcpConnection::sendInLoop()来实现发送数据
void TcpConnection::send(const std::string& message){
    if(state_ == KConnected){
        if(loop_->isInLoopThread()){
            // sendInLoop() first, then handleWrite() by writeCallback
            sendInLoop(message);
        }
        else{
            //函数指针fp指向sendInLoop;
            void (TcpConnection::*fp)(const string& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, message.data()));
        }
    }
}

void TcpConnection::send(const void* data, int len){
    send(std::string(static_cast<const char*>(data), len));
}

void TcpConnection::send(Buffer* buf){
    if(state_ == KConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else{
            void (TcpConnection::*fp)(const string& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
        }
    }
}

void TcpConnection::sendInLoop(const string& message)
{
    sendInLoop(message.data(), message.size());
}

//在EventLoop中回调这个sendInLoop(),用于发送message数据
//内部通过sockets::write()实现发送数据,先发送一次,如果没全部发送完,把数据先
//放到outputbuffer中,让channel注册写网络事件,等到内核可写时,socket就会出发可写
//网络事件,channel就回调handleWrite(),把outputbuffer中的数据发送出去
void TcpConnection::sendInLoop(const void* data, size_t len){
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;         //已经发了多少字节
    ssize_t remaining = 0;      //还剩下多少字节没发送
    bool faultError = false;    //错误
    if (state_ == KDisconnected){
        LOG_WARN << "disconnected, give up writing";
        return;
    }

    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0){
        nwrote = sockets::write(channel_->fd(), data, len);
        if(nwrote >= 0){
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }

            if(implicit_cast<size_t>(nwrote) < len){
                LOG_TRACE << "I am going to write some data.";
            }
        }
        else{   //写失败
            nwrote = 0;
            if(errno != EWOULDBLOCK){
                LOG_SYSERR << "TcpConnection::sendInLoop()";
                if(errno == EPIPE || errno == ECONNRESET || errno == EBADF || errno == EFAULT || errno == EINTR){
                    faultError = true;
                }
            }
        }
    }
    
    // data still available
    assert(nwrote >= 0);
    if(!faultError && remaining > 0){
        //还剩下多少字节没写出去
        size_t oldLen = outputBuffer_.readableBytes();
        //发生特定条件,让EventLoop回调highWaterMarkCallback_
        if(oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_){
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        //让 写缓冲区 追加 没写完的数据,并设置 channel_注册写网络事件,用于下次继续写
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        if(!channel_->isWriting()){
            channel_->enableWriting();
        }
    }
}

//关闭tcp连接,并让EventLoop回调&TcpConnection::shutdownInLoop()
void TcpConnection::shutdown(){
    // different from the forceClose, if considering KDisconnecting, it may be shutdown twice
    //只有已经建立了连接才可以关闭连接
    if(state_ == KConnected){
        setState(KDisconnecting);
        // considering the cost is it worth comparing twice? 
        // __sync_compare_and_swap(&state_, KConnected, KDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

//关闭TcpConnection,这个函数由EventLoop调用
void TcpConnection::shutdownInLoop(){
    //保证EventLoop在其所在的线程而非当前线程调用该函数
    loop_->assertInLoopThread();
    //关闭套接字写操作
    if(!channel_->isWriting()){
        socket_->shutdownWrite();
    }
}

//让EventLoop强制关闭tcp连接,利用EventLoop的定时器队列实现(唤醒epoller::dispatch()来关闭)
//回调TcpConnection::forceCloseInLoop()
void TcpConnection::forceClose(){
    __sync_bool_compare_and_swap(&state_, KConnected, KDisconnecting);

    if(state_ == KDisconnecting){
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

//强制在seconds秒后关闭tcp连接
// void TcpConnection::forceCloseWithDelay(double seconds)
// {
//     __sync_bool_compare_and_swap(&state_, KConnected, KDisconnecting);
//     void (TcpConnection::*fp)() = &TcpConnection::forceClose;
//     loop_->runAfter(seconds, std::bind(&TcpConnection::forceClose, shared_from_this()));
// }

//供EventLoop回调使用,用来强制关闭tcp连接
void TcpConnection::forceCloseInLoop(){
    loop_->assertInLoopThread();
    if(state_ == KConnected || state_ == KDisconnecting){
        handleClose();
    }
}

const char* TcpConnection::stateToString() const
{
  switch (state_)
  {
    case KDisconnected:
      return "kDisconnected";
    case KConnecting:
      return "kConnecting";
    case KConnected:
      return "kConnected";
    case KDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

//sockets::setTcpNoDelay()
void TcpConnection::setTcpNoDelay(bool on){
    socket_->setTcpNoDelay(on);
}

//EventLoop回调TcpConnection::startReadInLoop()
void TcpConnection::startRead(){
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}
//让channel注册读网络事件
void TcpConnection::startReadInLoop(){
    loop_->assertInLoopThread();
    if (!reading_ || !channel_->isReading()){
        channel_->enableReading();
        reading_ = true;
    }
}
//EventLoop回调TcpConnection::stopReadInLoop()
void TcpConnection::stopRead(){
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}
//让channel注销读网络事件
void TcpConnection::stopReadInLoop()
{
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading()){
        channel_->disableReading();
        reading_ = false;
    }
}

//完成连接建立完成时操作
void TcpConnection::connectEstablished(){
    loop_->assertInLoopThread();
    assert(state_ == KConnecting);
    setState(KConnected);

    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed(){
    loop_->assertInLoopThread();
    if(state_ == KConnected){
        setState(KDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }

    // the same as loop_->removeChannel(get_pointer(channel_));
    channel_->remove();
}

//处理读网络事件,供channel回调,把数据读到inputBuffer_中
void TcpConnection::handleRead(Timestamp t){
    int savedErrno = 0;
    //调用sockets::readv()把数据读到inputBuffer_中
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);

    if(n > 0){
        messageCallback_(shared_from_this(), &inputBuffer_, t);
    }
    else if(n == 0){
        handleClose();
    }
    else{
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

//处理写网络事件,把outputBuffer_中的数据写到channel_->fd()上,即发送数据
void TcpConnection::handleWrite(){
    loop_->assertInLoopThread();
    if(channel_->isWriting()){
        ssize_t n = sockets::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if(n > 0){  //成功写入的字节数
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0){
                channel_->disableWriting();
                if(writeCompleteCallback_){
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }

                if(state_ == KDisconnecting){
                    shutdownInLoop();
                }
            }
        }
        else{
            LOG_SYSERR << "TcpConnection::handleWrite()";
        }
    }
    else{
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}

//处理关闭连接的网络事件,让channel不再关心任何网络事件
//主动调用TcpConnection::forceClose()或者远方断开连接都会触发这个回调
void TcpConnection::handleClose(){
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == KConnected || state_ == KDisconnecting);
    
    setState(KDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    closeCallback_(guardThis);
}

//处理错误事件,利用sockets::getSocketError()来实现
void TcpConnection::handleError(){
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}


