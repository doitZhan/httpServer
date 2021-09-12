///缓冲区类模仿 org.jboss.netty.buffer.ChannelBuffer
/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |    预留的空间       | 数据(CONTENT)    | 可写的空间         |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// 开头               可读位置          可写位置               结尾
/// @endcode
///
/// 注意添加数据时,如果剩余空间可用,直接添加,否则若是剩余空间+预留空间可用,
/// 把数据挪动到kCheapPrepend位置即可,否则就需要额外扩容了

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "StringPiece.h"
#include "Endian.h"
#include <algorithm>
#include <vector>
#include <assert.h>
#include <string.h>

class Buffer{
public:
    //总空间1024+8=1032
    static const size_t KCheapPrepend = 8;      //一开始预留空间为8字节
    static const size_t KInitialSize = 1024;    //一开始初始空间1024

    //explicit 禁止隐式转换
    //readerIndex_和writerIndex_初始都在KCheapPrepend(8)处,buffer_大小(1032)
    explicit Buffer(size_t initialSize = KInitialSize)
        :buffer_(KCheapPrepend + initialSize),
        readerIndex_(KCheapPrepend),
        writerIndex_(KCheapPrepend){
            assert(readableBytes() == 0);
            assert(writableBytes() == initialSize);
            assert(prependableBytes() == KCheapPrepend);
    }

    //交换两个buffer类,实际上完成buffer_,readerIndex_,writerIndex_的交换
    void swap(Buffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    //当前buffer_可读的字节数,从可读位置到可写位置
    size_t readableBytes() const{
        return writerIndex_ - readerIndex_;
    }

    //当前buffer_可写的字节数,从可写位置到尾处
    size_t writableBytes() const{
        return buffer_.size() - writerIndex_;
    }

    //当前预留的字节数,从0到可读位置
    size_t prependableBytes() const{
        return readerIndex_;
    }

    //得到可读位置的地址
    const char* peek() const{
        return begin() + readerIndex_;
    }

    //左闭右开可读区间[peek(), beginWrite()),在此区间中找到[kCRLF,kCRLF+2)的地址
    const char* findCRLF() const{
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    //在[start,beginWrite())中找到第一个"\r\n"的位置
    const char* findCRLF(const char* start) const{
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite()? NULL : crlf;
    }

    //找到换行符'\n'的地址
    const char* findEOL() const
    {
        //从peek()所指区域的前readbleBytes()个字符中找到'\n';
        const void* eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

    //从start位置开始,找到换行符'\n'的地址
    const char* findEOL(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const void* eol = memchr(start, '\n', beginWrite() - start);
        return static_cast<const char*>(eol);
    }

    //从peek()可读位置开始,让readerIndex_移动(索回)len个位置,增加prependableBytes
    void retrieve(size_t len){
        assert(len <= readableBytes());
        if(len < readableBytes()){
            readerIndex_ += len;
        }
        else{
            retrieveAll();
        }
    }

    //从peek()可读位置开始,让readerIndex_移动直到end为止
    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    //让readerIndex_移动sizeof(int_*)个位置
    void retrieveInt64()
    {
        retrieve(sizeof(int64_t));
    }
    void retrieveInt32()
    {
        retrieve(sizeof(int32_t));
    }
    void retrieveInt16()
    {
        retrieve(sizeof(int16_t));
    }
    void retrieveInt8()
    {
        retrieve(sizeof(int8_t));
    }

    //让读位置和写位置都回到kCheapPrepend
    void retrieveAll(){
        readerIndex_ = KCheapPrepend;
        writerIndex_ = KCheapPrepend;
    }

    //读取len个字节的数据并移动readerIndex_
    string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        string result(peek(), len);
        retrieve(len);
        return result;
    }

    //读取剩余所有数据并移动readerIndex_
    string retrieveAllAsString(){
        int length = readableBytes();
        string result(peek(), length);
        retrieve(length);
        return result;
    }

    //读取剩余所有的数据并移动readerIndex_
    string toString() const
    {
        return string(peek(), static_cast<int>(readableBytes()));
    }

    //往可写区域追加data，移动writerIndex_
    void append(const char* data, size_t len){
        ensureWritableBytes(len);    //确保buf的可写区域能够容纳data数据内容
        std::copy(data, data + len, beginWrite());    //把data拷贝到buf的可写区域中
        hasWritten(len);    //拷贝后移动writerIndex_
    }

    void append(const StringPiece& str){
        append(str.data(), str.size());
    }

    void append(const void* data, size_t len){
        append(static_cast<const char*>(data), len);
    }

    //剩余空间不足len时,对buffer_进行扩容或者挪动数据,保证能写len字节的数据
    void ensureWritableBytes(size_t len){
        if(writableBytes() < len){
            makeSpace(len);    //可写区域不够，扩容
        }
        assert(writableBytes() >= len);
    }

    //返回可写位置的地址
    char* beginWrite(){
        return begin() + writerIndex_;
    }

    const char* beginWrite() const{
        return begin() + writerIndex_;
    }

    //更新writerIndex_ + len
    void hasWritten(size_t len){
        assert(len <= writableBytes());
        writerIndex_ += len;
    }

    //更新writerIndex_ - len
    void unwrite(size_t len){
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }

    //向buffer_中写网络字节序int_*,改变writerIndex_位置
    void appendInt64(int64_t x){
        int64_t be64 = hostToNetwork64(x);
        append(&be64, sizeof(be64));
    }
    void appendInt32(int32_t x){
        int32_t be32 = hostToNetwork32(x);
        append(&be32, sizeof be32);
    }
    void appendInt16(int16_t x){
        int16_t be16 = hostToNetwork16(x);
        append(&be16, sizeof be16);
    }
    void appendInt8(int8_t x){
        append(&x, sizeof x);
    }

    //从buffer_中可读位置返回第一个in64_*的网络字节序,不会改变readerIndex_位置
    int64_t peekInt64() const{
        assert(readableBytes() >= sizeof(int64_t));
        int64_t be64 = 0;
        ::memcpy(&be64, peek(), sizeof be64);
        return networkToHost64(be64);
    }
    int32_t peekInt32() const{
        assert(readableBytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, peek(), sizeof be32);
        return networkToHost32(be32);
    }
    int16_t peekInt16() const{
        assert(readableBytes() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, peek(), sizeof be16);
        return networkToHost16(be16);
    }
    int8_t peekInt8() const{
        assert(readableBytes() >= sizeof(int8_t));
        int8_t x = *peek();
      return x;
    }

    //从buffer_中读取一个int64_t,返回主机字节序,会移动readerIndex_
    int64_t readInt64(){
        //获取8个字节的数据,返回主机字节序
        int64_t result = peekInt64();
        retrieveInt64();    //移动readerIndex_
        return result;
    }
    int32_t readInt32(){
        //获取4个字节的数据,返回主机字节序
        int32_t result = peekInt32();
        retrieveInt32();    //移动readerIndex_
        return result;
    }
    int16_t readInt16(){
        int16_t result = peekInt16();
        retrieveInt16();
        return result;
    }
    int8_t readInt8(){
        int8_t result = peekInt8();
        retrieveInt8();
        return result;
    }

    //向预留空间中写入数据,从readerIndex_向前也就是向左拷贝data
    void prepend(const void* data, size_t len){
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d + len, begin() + readerIndex_);
    }

    void prependInt64(int64_t x){
        //获取网络字节序
        int64_t be64 = hostToNetwork64(x);
        prepend(&be64, sizeof(be64));   //向预留空间中写入8个字节的数据
    }

    //同上,只是字节数稍有变化
    void prependInt32(int32_t x){
        int32_t be32 = hostToNetwork32(x);
        prepend(&be32, sizeof be32);
    }
    void prependInt16(int16_t x){
        int16_t be16 = hostToNetwork16(x);
        prepend(&be16, sizeof be16);
    }

    void prependInt8(int8_t x){
        prepend(&x, sizeof x);
    }

    //把数据读到另一个buffer里,并进行交换
    void shrink(size_t reserve){
        Buffer other;
        //保证可用空间足够,扩容/挪动数据
        other.ensureWritableBytes(readableBytes() + reserve);
        other.append(toString());
        swap(other);
    }

    //得到buffer_的已分配空间大小
    size_t internalCapacity() const{
        return buffer_.capacity();
    }

    ssize_t readFd(int fd, int* savedErrno);

    StringPiece toStringPiece() const{
        return StringPiece(peek(), static_cast<int>(readableBytes()));
    }
private:
    //初始位置的地址
    char* begin(){
        return &*buffer_.begin();
    }
    const char* begin() const{
        return &*buffer_.begin();
    }

    //buffer_扩容/数据挪动操作
    void makeSpace(size_t len){
        // if(writableBytes() + prependableBytes() < len + KCheapPrepend){
        //     // improment: use move to avoid initialize cost 
        //     buffer_.resize(writerIndex_ + len);
        // }
        // else{
        //     assert(KCheapPrepend < readerIndex_);
        //     size_t readable = readableBytes();
        //     std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + KCheapPrepend);
        //     readerIndex_ = KCheapPrepend;
        //     writerIndex_ = readerIndex_ + readable;
        //     assert(readable == readableBytes());
        // }
        if(KCheapPrepend < readerIndex_){    //readerIndex_往后推移，把readerIndex_重新置位到KCheapPrepend，避免扩容后KCheapPrepend到readerIndex_的空间浪费
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + KCheapPrepend);    //移动写入的可读区域
            readerIndex_ = KCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }

        if(writableBytes() < len){
            buffer_.reserve(writerIndex_ + len);    //reserve增加了vector的capacity，但是它的size没有改变，对容器原有的元素没有影响。
        }
    }
    
    std::vector<char> buffer_;      //保存缓冲区数据
    size_t readerIndex_;            //读下标
    size_t writerIndex_;            //写下标
    static const char kCRLF[];      //"\r\n"
};

#endif
