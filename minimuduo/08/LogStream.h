#ifndef __LOGSTREAM_H__
#define __LOGSTREAM_H__

#include "nonCopyable.h"
#include "StringPiece.h"

#include <memory>

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template<int SIZE> //变量SIZE是只能使用int类型
class FixedBuffer : public nonCopyable{
public:
    FixedBuffer()
        :cur_(data_){

    }

    const char* data() const{
        return data_;
    }

    char* current(){
        return cur_;
    }

    int length() const{
        return static_cast<int>(cur_ - data_);
    }

    int avail() const{
        return static_cast<int>(end() - cur_);
    }

    void append(const char* buf, size_t len){
        if (static_cast<size_t>(avail()) > len){
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    void add(size_t len){
        cur_ += len;
    }

    void reset(){
        cur_ = data_;
    }

    void bzeroInner(){
        bzero(data_, sizeof(data_));
    }

private:
    const char* end() const{
        return data_ + sizeof(data_);
    }

    char data_[SIZE];
    char *cur_;
};


class LogStream : public nonCopyable{
public:
    typedef LogStream self;
    typedef FixedBuffer<kSmallBuffer> Buffer;

    // left value
    self& operator<<(bool v){
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);

    self& operator<<(const void*);

    self& operator<<(float v){
        *this << static_cast<double>(v);
        return *this;
    }
    self& operator<<(double);

    self& operator<<(char v){
        buffer_.append(&v, 1);
        return *this;
    }

    self& operator<<(const char* str){
        if(str){
            buffer_.append(str, strlen(str));
        }
        else{
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    self& operator<<(const unsigned char* str){
        return operator<<(reinterpret_cast<const char*>(str));
    }

    self& operator<<(const std::string& v){
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    self& operator<<(const StringPiece& v){
        buffer_.append(v.data(), v.size());
        return *this;
    }

    void append(const char* data, int len){
        buffer_.append(data, len);
    }

    const Buffer& buffer() const{
        return buffer_;
    }

    void resetBuffer(){
        buffer_.reset();
    }

private:
    void staticCheck();

    template<typename T>
    void formatInteger(T);

    Buffer buffer_;
    static const int kMaxNumericSize = 32;
};

class Fmt{
public:
    template<typename T>
    Fmt(const char* fmt, T val);

    const char* data() const {
        return buf_;
    }

    int length() const{
        return length_;
    }
private:
    char buf_[32];
    int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt){
    s.append(fmt.data(), fmt.length());
    return s;
}


#endif