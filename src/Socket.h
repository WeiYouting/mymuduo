#pragma once

#include "InetAddress.h"
#include "noncopyable.h"

// 封装socket fd
class Socket:noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
        {}
    ~Socket();

    int fd() const {return this->sockfd_;}
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

    int getSocketError();

private:
    const int sockfd_;
};