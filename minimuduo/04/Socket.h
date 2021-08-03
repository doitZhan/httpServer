#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "nonCopyable.h"

struct tcp_info;
class InetAddress;

class Socket : public nonCopyable{
public:
    explicit Socket(int sockfd)
        :sockfd_(sockfd){

    }
    ~Socket();

    int fd() const{
        return sockfd_;
    }

    bool getTcpInfo(struct tcp_info*) const;
    bool getTcpInfoString(char *buf, int len) const;

    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress *peeraddr);
    void shutdownWrite();

    //set sockfd operation
    void setTcpNoDelay(bool on); //nagle's algorithm
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};

#endif