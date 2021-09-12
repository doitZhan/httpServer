#include "../../net/Buffer.h"
#include "HttpContext.h"


bool HttpContext::processRequestLine(const char *begin, const char *end){
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if (space != end && request_.setMethod(start, space)){    //获取请求method，存入request_对象
        start = space + 1;
        space = std::find(start, end, ' ');    //找到' '并返回' '所在指针地址
        if (space != end){
            const char *queryPos = std::find(start, space, '?');
            if (queryPos != space){
                request_.setQuery(queryPos, space);    //获取请求query参数，存入request_对象
            }
            request_.setPath(start, queryPos);    //获取请求path参数，存入request_对象
            start = space + 1;
            succeed = ((end - start == 8) && (std::equal(start, end - 1, "HTTP/1.")));    //获取请求version参数，存入request_对象
            if (succeed){
                if (*(end - 1) == '1'){
                    request_.setVersion(HttpRequest::kHttp11);
                }
                else if(*(end - 1) == '0'){
                    request_.setVersion(HttpRequest::kHttp10);
                }
                else{
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime){
    bool ok = true;
    bool hasMore = true;    //Http请求消息分为请求行、请求头以及请求体三部分，全部解析完成或部分解析失败，设置为false跳出while循环
    while (hasMore){
        if (state_ == kExpectRequestLine){  //解析请求行
            const char *crlf = buf->findCRLF();    //查找buf的CRLF
            if (crlf){
                ok = processRequestLine(buf->peek(), crlf);    //传入请求行的头指针和尾指针
                if (ok){
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);    //请求行解析完毕，在buf中移动readerIndex
                    state_ = kExpectHeaders;    //请求行解析成功，切换状态为解析请求头
                }
                else{
                    hasMore = false;    //请求行解析失败，hasMore设置为false跳出循环
                }
            }
            else{
                hasMore = false;    //请求行解析失败，hasMore设置为false跳出循环
            }
        }
        else if (state_ == kExpectHeaders){    //解析请求头
            const char *crlf = buf->findCRLF();    //查找buf的CRLF
            if (crlf){
                const char *colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf){
                    request_.addHeader(buf->peek(), colon, crlf);    //解析请求头，存入request_对象中
                }
                else{
                    state_ = kGotAll;    //全部解析完毕
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2);    //请求头解析成功，在buf中移动readerIndex
            }
            else{
                hasMore = false;
            }
        }
        else if(state_ == kExpectBody){

        }
    }
    return ok;
}