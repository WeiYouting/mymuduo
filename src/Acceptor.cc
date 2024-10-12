#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"
#include "Channel.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

static int createNonBlocking()
{
    int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
    if(sockfd < 0){
        LOG_FATAL("%s:%s:%d listen socket create error:%d",__FILE__,__FUNCTION__,__LINE__,errno);
    }  
}

Acceptor::Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport)
    :loop_(loop)
    ,acceptSocket_(createNonBlocking())
    ,acceptChannel_(loop,this->acceptSocket_.fd())
    ,listenning_(false)
{
    this->acceptSocket_.setReuseAddr(true);
    this->acceptSocket_.setReusePort(true);
    this->acceptSocket_.bindAddress(listenAddr);

    this->acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));

}

Acceptor::~Acceptor()
{
    this->acceptChannel_.disableAll();
    this->acceptChannel_.remove();
}

void  Acceptor::listen()
{
    this->listenning_ = true;
    this->acceptSocket_.listen();
    this->acceptChannel_.enableReading();
}

// listenfd有事件发生，有新用户连接了
void  Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd = this->acceptSocket_.accept(&peerAddr);
    if(connfd >= 0){
        if(this->newConnectionCallback_){
            this->newConnectionCallback_(connfd,peerAddr);
        }else{
            ::close(connfd);
        }
    }else{
        LOG_ERROR("%s:%s:%d accept error:%d",__FILE__,__FUNCTION__,__LINE__,errno);
        if(errno = EMFILE){
            LOG_ERROR("%s:%s:%d sockfd reached limit",__FILE__,__FUNCTION__,__LINE__);
        }
    }
}