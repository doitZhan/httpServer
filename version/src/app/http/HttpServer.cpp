#include "HttpServer.h"
#include "../../net/Logging.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

void defaultHttpCallback(const HttpRequest& reqt, HttpResponse* resp){    //默认HTTP回调，返回404错误码
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name)
    :server_(loop, listenAddr, name),
    httpCallback_(defaultHttpCallback){
        server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, std::placeholders::_1));    //注册处理连接回调函数
        server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));    //注册消息处理回调函数
}

HttpServer::~HttpServer(){

}

void HttpServer::start(){    //启动httpServer服务器
    LOG_WARN << "HttpServer[" << server_.name()
             << "] starts listening on " << server_.ipPort();
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn){    //处理新连接到来回调函数
    if(conn->connected()){
        conn->setContext(HttpContext());    //在conn对象设置一个HTTP协议解析context
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp receiveTime){    //处理请求消息回调函数
    HttpContext* context = &((*conn->getNonConstContext()).anyCast<HttpContext>());
    if(!context->parseRequest(buf, receiveTime)){
        conn->send("Http/1.1 400 Bad Request\r\n\r\n");    //请求消息解析失败，返回400并关闭TCP连接
        conn->shutdown();    //关闭写这一半，此时用户不能再向套接字中写数据，内核会把缓存中的数据发送出去，接着不会再发送数据
    }
    if(context->isGotAll()){
        onRequest(conn, context->getRequest());    //请求消息解析成功，处理请求消息
        context->reset();    //协议解析完毕，重新置位，conn要重复利用context
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req){
    const std::string& connection = req.getAHeader("Connection");
    bool close = (connection == "close" || (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive"));
    HttpResponse response(close);
    httpCallback_(req, &response);    //调用回调函数构造响应消息
    
    Buffer buf;
    response.appendToBuffer(&buf);    //将响应消息写入buf
    conn->send(&buf);    //通过conn发送buf给对端
    if(response.getCloseConnection()){
        conn->shutdown();    //判断是否需要关闭TCP连接
    }
}



