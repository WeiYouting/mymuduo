#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    ::close(this->sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{   
    if(0 != ::bind(this->sockfd_,(sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in))){
        LOG_FATAL("bind sockfd:%d fail",this->sockfd_);
    }
}

void Socket::listen()
{
    if(0 != ::listen(this->sockfd_,1024)){
        LOG_FATAL("listen sockfd:%d fail",this->sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::bzero(&addr,sizeof(addr));
    int connfd = ::accept4(this->sockfd_,(sockaddr*)&addr,&len,SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >= 0){
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if(::shutdown(this->sockfd_,SHUT_WR) < 0){
        LOG_ERROR(" Socket::shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(this->sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
       int optval = on ? 1 : 0;
    ::setsockopt(this->sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval)); 
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(this->sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval)); 
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(this->sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval)); 
}

int Socket::getSocketError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    if(::getsockopt(this->sockfd_,SOL_SOCKET,SO_ERROR,&optval,&optlen) < 0){
        return errno;
    }
    return optval;
}