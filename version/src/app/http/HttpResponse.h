/*
    http响应类。
消息格式示例:
HTTP/1.1 404 Not Found
Date: Mon, 06 Mar 2006 09:03:14 GMT
Server: Apache/2.0.55 (Unix) PHP/5.0.5
Content-Length: 291
Keep-Alive: timeout=15, max=100
Connection: Keep-Alive
Content-Type: text/html; charset=iso-8859-1
 
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html><head>
<title>404 Not Found</title>
</head><body>
<h1>Not Found</h1>
<p>The requested URL /notexist was not found on this server.</p>
<hr>
<address>Apache/2.0.55 (Unix) PHP/5.0.5 Server at localhost Port 8080</address>
</body></html>
*/

#ifndef __HTTPRESPONSE_H__
#define __HTTPRESPONSE_H__

#include "../../net/StringPiece.h"

#include <map>

class Buffer;

class HttpResponse{
public:
    enum HttpStatusCode{    //http响应码枚举
        kUnknown,
        k200Ok = 200,
        k301MovePermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404
    };

    explicit HttpResponse(bool close)
        :statusCode_(kUnknown),
        closeConnection_(close){

    }

    void setStatusCode(HttpStatusCode code){
        statusCode_ = code;
    }

    void setStatusMessage(const StringPiece& message){
        statusMessage_ = message;
    }

    void setCloseConnection(bool on){
        closeConnection_ = on;
    }
    bool getCloseConnection()const{
        return closeConnection_;
    }

    void addHeader(const StringPiece& key, const StringPiece& value){
        headers_[key] = value;
    }
    void setContentType(const StringPiece& contentType){
        addHeader("Content-Type", contentType);
    }

    void setBody(const StringPiece& body){
        body_ = body;
    }

    void appendToBuffer(Buffer *output) const;    //往buf中写入响应消息

private:
    std::map<StringPiece, StringPiece> headers_;
    HttpStatusCode statusCode_;
    StringPiece statusMessage_;
    bool closeConnection_;
    StringPiece body_;
};




#endif