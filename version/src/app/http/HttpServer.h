/*
    HTTP服务器类。
*/

#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include "../../net/TcpServer.h"
#include <memory>

class HttpRequest;
class HttpResponse;

class HttpServer : public nonCopyable{
public:
    typedef std::function<void (const HttpRequest&, HttpResponse*)> HttpCallback;
    
    HttpServer(EventLoop *loop, const InetAddress& listenAddr, const std::string& name);
    ~HttpServer();

    EventLoop* getLoop() const{
        return server_.getLoop();
    }

    void setHttpCallback(const HttpCallback& cb){    //设置消息处理回调函数
        httpCallback_ = cb;
    }

    void setThreadNum(int numThreads){    //开启多线程时设置线程数量
        server_.setThreadNum(numThreads);
    }

    void start();    //回调函数、线程数量设置好以后，才能启动tcp
    
private:
    TcpServer server_;
    HttpCallback httpCallback_;    //消息处理逻辑提交给回调函数来处理

    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp receiveTime);
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);
};



#endif