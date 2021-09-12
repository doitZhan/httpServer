#include "Buffer.h"
#include "SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

template<typename To, typename From>
inline To implicit_cast(From const& f){
    return f;
}

const char Buffer::kCRLF[] = "\r\n";
const size_t Buffer::KCheapPrepend;
const size_t Buffer::KInitialSize;

ssize_t Buffer::readFd(int fd, int* savedErrno){
    //栈空间,用于从套接字中读数据时,buffer_不够用时暂存数据,等到buffer_足够时拷贝到buffer_
    // when buffer space is not enough, write in the extrabuf first, then write to the buffer, to save an ioctl()/FIONREAD call cost
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base =  begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf)? 2 : 1;
    // multi-buffer read operation
    const ssize_t n = sockets::readv(fd, vec, iovcnt);
    if(n < 0){
        *savedErrno = errno;
    }
    else if(implicit_cast<size_t>(n) <= writable){
        writerIndex_ += n;
    }
    else{
        //把额外的extra部分添加到buffer_中
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    
    return n;
}


