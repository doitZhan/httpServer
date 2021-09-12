/*
    http请求类。
*/

#ifndef __HTTPREQYEST_H__
#define __HTTPREQYEST_H__

#include "../../net/Timestamp.h"
#include <map>
#include <string>
#include <assert.h>
#include <stdio.h>

class HttpRequest{
public:
    enum Method{    //Http请求方法枚举-method
        kInvalid, kGet, kPost, kHead, kPut, kDelete
    };

    enum Version{    //Http版本枚举-version
        kUnknown, kHttp10, kHttp11
    };

    HttpRequest()
        :method_(kInvalid),
        version_(kUnknown){

    }

    void setVersion(Version v){
        version_ = v;
    }
    Version getVersion() const{
        return version_;
    }

    bool setMethod(const char *start, const char *end){
        assert(method_ == kInvalid);
        std::string m(start, end);  //使用字符串首尾构造string，不包括尾部，如char *s="123", string s=(s,s+3),则s输出为123

        if (m == "GET"){
            method_ = kGet;
        }
        else if (m == "POST"){
            method_ = kPost;
        }
        else if(m == "HEAD"){
            method_ = kHead;
        }
        else if(m == "PUT"){
            method_ = kPut;
        }
        else if(m == "DELETE"){
            method_ = kDelete;
        }
        else{
            method_ = kInvalid;
        }
        return method_ != kInvalid;
    }
    Method getMethod() const{
        return method_;
    }

    void setPath(const char* start, const char* end){
        path_.assign(start, end);
    }
    const std::string& getPath() const{
        return path_;
    }

    void setQuery(const char* start, const char* end){
        query_.assign(start, end);    //使用字符串首尾重新赋值string，不包括尾部
    }
    const std::string& getQuery() const{
        return query_;
    }

    void setReceiveTime(Timestamp t){
        receiveTime_ = t;
    }
    Timestamp getReceiveTime() const{
        return receiveTime_;
    }
    
    // 解析请求头 (key, value) ，存入headers_
    void addHeader(const char* start, const char* colon, const char* end){
        std::string field(start, colon);
        ++colon;
        while(colon < end && isspace(*colon)){
            ++colon;
        }
        std::string value(colon, end);
        while(!value.empty() && isspace(value[value.size() - 1])){
            value.resize(value.size() - 1);
        }
        headers_[field] = value;
    }
    
    std::string getAHeader(const std::string& field) const{
        std::string result;
        std::map<std::string, std::string>::const_iterator it = headers_.find(field);
        if(it != headers_.end()){
            result = it->second;
        }
        return result;
    }

    const std::map<std::string, std::string>& getHeaders() const{
        return headers_;
    }

    void swap(HttpRequest& hr){
        std::swap(method_, hr.method_);
        std::swap(version_, hr.version_);
        path_.swap(hr.path_);
        query_.swap(hr.query_);
        receiveTime_.swap(hr.receiveTime_);
        headers_.swap(hr.headers_);
    }

private:
    Method method_;
    Version version_;
    std::string path_;
    std::string query_;
    Timestamp receiveTime_;
    std::map<std::string, std::string> headers_;
};



#endif