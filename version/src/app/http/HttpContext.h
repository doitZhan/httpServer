/*
    HTTP协议解析类。
    针对http包进行处理，在处理过程中存在四种状态间切换。

http协议举例：
GET /simple.html HTTP/1.1<CRLF>     ----- 首行
Accept: text/html<CRLF>             --|
Accept-Language: zh-cn<CRLF>          |
Accept-Encoding: gzip, deflate<CRLF>  |-- 头部
User-Agent: Mozilla/4.0<CRLF>         |
Host: localhost:8080<CRLF>            |
Connection: Keep-Alive<CRLF>        --|
<CRLF>                              ----- 空白行表示头部的结束
                                    ----- 接下来的内容是正文部分
*/

#ifndef __HTTPCONTEXT_H__
#define __HTTPCONTEXT_H__

#include "HttpRequest.h"

class Buffer;

class HttpContext{
public:
    enum HttpRequestParseState{  //解析请求状态的枚举常量
        kExpectRequestLine,      //解析请求行状态
        kExpectHeaders,          //解析请求头状态，key-value
        kExpectBody,             //解析请求实体状态
        kGotAll                  //解析完毕状态
    };
    HttpContext()
        :state_(kExpectRequestLine){   //默认初始状态为解析请求行状态

    }
    
    bool parseRequest(Buffer *buf, Timestamp receiveTime);  //解析Http请求消息

    bool isGotAll() const{           //是否解析完毕请求消息，根据请求解析状态判断
        return state_ == kGotAll;
    }

    void reset(){                    //重置HttpRequest对象
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& getRequest() const{  //获取HttpRequest对象
        return request_;
    }

    HttpRequest& getRequest(){              //函数重载，获取HttpRequest对象
        return request_;
    }
private:
    bool processRequestLine(const char *begin, const char *end);  // 解析Http请求行，method path version

    HttpRequestParseState state_;
    HttpRequest request_;
};


#endif