#include "HttpResponse.h"
#include "../../net/Buffer.h"

void HttpResponse::appendToBuffer(Buffer *output) const{
    StringPiece crlf("\r\n");
    char buf[32];
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d", statusCode_);    //构造响应行，写入buf
    output->append(StringPiece(buf));
    output->append(StringPiece(statusMessage_));
    output->append(crlf);

    if (closeConnection_){
        output->append(StringPiece("Connection: close\r\n"));    //写入响应头
    }
    else{
        snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", static_cast<size_t>(body_.size()));
        output->append(StringPiece(buf));
        output->append(StringPiece("Connection: Keep-Alive\r\n"));
    }

    for(std::map<StringPiece, StringPiece>::const_iterator it = headers_.begin(); it != headers_.end(); it++){
        output->append(it->first);
        output->append(StringPiece(": "));
        output->append(it->second);
        output->append(crlf);
    }

    output->append(crlf);
    output->append(body_);    //写入响应消息体
}