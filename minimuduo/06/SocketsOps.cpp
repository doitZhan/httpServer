#include "SocketsOps.h"
#include "Endian.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <strings.h>
// safe upcast: static_cast and const_cast
template<typename To, typename From>
inline To implicit_cast(From const &f){
    return f;
}

using namespace sockets;

namespace sockets{

typedef struct sockaddr SA;

const struct sockaddr* sockaddr_cast(const struct sockaddr_in *addr){
    return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

const struct sockaddr* sockaddr_cast(const struct sockaddr_in6 *addr){
    return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

struct sockaddr* sockaddr_cast(struct sockaddr_in6 *addr){
    return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr *addr){
    return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr *addr){
    return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(addr));
}

int createNonblockingOrDie(sa_family_t family){
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK |SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0){

    }
    return sockfd;
}

int connect(int sockfd, const struct sockaddr *addr){
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

void bindOrDie(int sockfd, const struct sockaddr *addr){
    int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (ret < 0){

    }
}

void listenOrDie(int sockfd){
    int ret = ::listen(sockfd, SOMAXCONN);
    if(ret < 0){
        
    }
}

int accept(int sockfd, struct sockaddr_in6* addr){
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
    
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd < 0){
        int savedErrno = errno;
        // lOG_SYSERR << "accpet in SocketsOps.cpp";
        
        // the meaning of the errno??
        switch (savedErrno){
            // temporary error
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
                // expected errors
                errno = savedErrno;
                break;
            // fatal error
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                // LOG_FATAL << "unexpected error of accpet "<< savedErrno;
                break;
            default:
                // LOG_FATAL << "unexpected error of accpet "<< savedErrno;
                break;
        }
    }
    return connfd;
}

ssize_t read(int sockfd, void* buf, size_t count){
    return ::read(sockfd, buf, count);
}

ssize_t readv(int sockfd, const struct iovec* iov, int iovcnt){
    return ::readv(sockfd, iov, iovcnt);
}

ssize_t write(int sockfd, const void* buf, size_t count){
    return ::write(sockfd, buf, count);
}

void close(int sockfd){
    if(::close(sockfd) < 0){
        // LOG_SYSERR << "close in SocketsOps.cpp";
    }
}

void shutdownWrite(int sockfd){
    if(::shutdown(sockfd, SHUT_WR) < 0){
        // LOG_SYSERR << "shiutdownWrite in SockertsOps.cpp";
    }
}

void toIp(char *buf, size_t size, const struct sockaddr *addr){
    if(addr->sa_family == AF_INET){
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
    else if(addr->sa_family == AF_INET6){
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
}

void toIpPort(char* buf, size_t size, const struct sockaddr* addr){
    toIp(buf, size, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
    uint16_t port = networkToHost16(addr4->sin_port);
    assert(size > end);
    snprintf(buf + end, size - end, "%u", port);
}

void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr){
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if(::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0){
        // LOG_SYSERR << "FromIpPort in SocketsOPs.cpp";
    }
}

void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in6 *addr){
    addr->sin6_family = AF_INET6;
    addr->sin6_port = hostToNetwork16(port);
    if(::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0){
        // LOG_SYSERR << "FromIpPort in SocketsOPs.cpp";
    }
}

int getSocketError(int sockfd){
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    // get options at the socket api level on sockets
    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0){
        return errno;
    }
    else{
        return optval;
    }
}

struct sockaddr_in6 getLocalAddr(int sockfd){
    struct sockaddr_in6 localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if(::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0){
        // LOG_SYSERR << "getLocalAddr in SocketsOps.cpp";
    }
    return localaddr;
}

struct sockaddr_in6 getPeerAddr(int sockfd){
    struct sockaddr_in6 peeraddr;
    bzero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if(::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0){
        // LOG_SYSERR << "getPeerAddr in SocketsOPs.cpp";
    }
    return peeraddr;
}

bool isSelfConnct(int sockfd){
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if(localaddr.sin6_family == AF_INET){
        const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
        const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }
    else if(localaddr.sin6_family == AF_INET6){
        return localaddr.sin6_port == peeraddr.sin6_port && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
    }
    else{
        return false;
    }
}

}