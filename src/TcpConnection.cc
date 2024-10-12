#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>


static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}


TcpConnection::TcpConnection(EventLoop *loop,const std::string& nameArg,int sockfd,
                                const InetAddress& localAddr,const InetAddress& peerAddr)
        :loop_(CheckLoopNotNull(loop))
        ,name_(nameArg)
        ,state_(kConnecting)
        ,reading_(true)
        ,socket_(new Socket(sockfd))
        ,channel_(new Channel(loop_,sockfd))
        ,localAddr_(localAddr)
        ,peerAddr_(peerAddr)
        ,highWaterMark_(64*1024*1024)
    {   
        channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
        channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
        channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
        channel_->setErrorCallback(std::bind(&TcpConnection::hanldeError,this));

        LOG_INFO("TcpConnection::ctor[%s] at fd=%d",this->name_.c_str(),sockfd);
        this->socket_->setKeepAlive(true);
    }

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d",this->name_.c_str(),this->channel_->fd(),(int)this->state_);
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    ssize_t n = this->inputBuffer_.readFd(this->channel_->fd(),&saveErrno);
    if(n > 0){
        // 已建立连接的用户，有可读事件发生
        messageCallback_(shared_from_this(),&this->inputBuffer_,receiveTime);
    }else if(n == 0){
        handleClose();
    }else{
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        hanldeError();
    }
}

void TcpConnection::handleWrite()
{
    if(this->channel_->isWriting()){
        int saveError = 0;
        ssize_t n = this->outputBuffer_.writeFd(this->channel_->fd(),&saveError);
        if(n > 0){
            this->outputBuffer_.retrieve(n);
            if(this->outputBuffer_.readableBytes() == 0){
                this->channel_->disableWriting();
                if(writeCompleteCallback_){
                    this->loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                if(this->state_ == kDisconnected){
                    this->shutdownInLoop();
                }
            }
        }else{
            LOG_ERROR("TcpConnection::handleWrite");     
        }
    }else{
        LOG_ERROR("TcpConnection fd=%d is down no more writing",this->channel_->fd());     
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n",this->channel_->fd(),(int)this->state_);
    this->setState(kDisconnected);
    this->channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());

    if(connectionCallback_){
        connectionCallback_(connPtr);
    }

    if(closeCallback_){
        closeCallback_(connPtr);
    }
   
}

void TcpConnection::hanldeError()
{
    int err = this->socket_->getSocketError();
    LOG_ERROR("TcpConnection::hanldeError name:%s - SO_ERROR:%d",this->name_.c_str(),err);
}

void TcpConnection::send(const std::string& buf){
    if(this->state_ == kConnected){
        if(this->loop_->isInLoopThread()){
            sendInLoop(buf.c_str(),buf.size());
        }else{
            this->loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));
        }
    }
}


void TcpConnection::sendInLoop(const void* data,size_t len){
    ssize_t nwrote = 0;
    ssize_t remaining = len;
    bool faultError = false;

    if(this->state_ == kDisconnected){
        LOG_ERROR("disconnected,give up writing");
        return;
    }

    if(!this->channel_->isWriting() && this->outputBuffer_.readableBytes() == 0){
        nwrote = ::write(this->channel_->fd(),data,len);
        if(nwrote >= 0){
            remaining = len - nwrote;
            // 一次性发送完成 不需要设置epollout事件
            if(remaining == 0 && writeCompleteCallback_){
                this->loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
            }
        }else{
            nwrote = 0;
            if(errno != EWOULDBLOCK){
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }

    // 数据没有全部发送完成，剩余数据需要保存在缓冲区，注册epollout事件
    if(!faultError && remaining > 0){
        // 目前缓冲区待发送数据的长度
        ssize_t oldLen = this->outputBuffer_.readableBytes();
        if(oldLen + remaining >= this->highWaterMark_ && oldLen < this->highWaterMark_ && highWaterMarkCallback_){
            this->loop_->queueInLoop(std::bind(highWaterMarkCallback_,shared_from_this(),oldLen+remaining));
        }
        this->outputBuffer_.append((char*)data + nwrote,remaining);
        if(!this->channel_->isWriting()){
            this->channel_->enableWriting();
        }
    }
}


void TcpConnection::shutdown()
{
    if(this->state_ == kConnected){
        this->setState(kDissConnecting);
        this->loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!this->channel_->isWriting()){
        this->socket_->shutdownWrite();  
    }
}

void TcpConnection::connectEstablished()
{
    this->setState(kConnected);
    this->channel_->tie(shared_from_this());
    this->channel_->enableReading();

    this->connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if(this->state_ == kConnected){
        this->setState(kDisconnected);
        this->channel_->disableAll();
        this->connectionCallback_(shared_from_this());
    }
    this->channel_->remove();
}