#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "Logging.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

//构造函数负责实现一个TCP服务端时创建套接字->绑定两个个步骤
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    :loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),
    acceptChannel_(loop_, acceptSocket_.fd()),
    listenning_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)){
        assert(idleFd_ > 0);
        //设置地址重用,端口重用,绑定Socket和listenAddr
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.setReusePort(reuseport);
        acceptSocket_.bindAddress(listenAddr);
        //设置读事件回调函数,一旦epoller::dispatcher发生读事件,channel就会回调这个handleRead来处理
        acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

//析构函数,负责关闭套接字,取消acceptChannel_上所有的网络事件,表示退出和不再接受处理任务网络事件
Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

//监听操作
void Acceptor::listen(){
    loop_->assertInLoopThread();    //保证EventLoop所在的线程是当前线程
    listenning_ = true;
    acceptSocket_.listen();
    //channel注册读网络事件,把这个channel更新到epoller中的epollfd_句柄上面
    acceptChannel_.enableReading();
}

//acceptChannel_处理读网络事件(连接到来),接受一个连接
void Acceptor::handleRead(){
    loop_->assertInLoopThread();
    //连接进来的客户端的套接字地址
    InetAddress peerAddr;
    //连接套接字:connfd
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0){    //accept成功
        if(newConnectionCallback_){ //回调连接成功处理函数
            newConnectionCallback_(connfd, peerAddr);
        }
        else{
            ::close(connfd);    //关闭套接字
        }
    }
    else{   //accept失败
        LOG_SYSERR << "in Acceptor::handleRead";
        if(errno == EMFILE){    //处理EMFILE失败
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            ::close(idleFd_);
            idleFd_ = ::open("dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}