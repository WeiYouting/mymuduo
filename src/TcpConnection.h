#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h" 
#include "Timestamp.h"

#include <atomic>
#include <memory>
#include <string>

class Channel;
class EventLoop;
class Socket;

/*  
 *  TcpServer => Acceptor 有新用户连接 获取connfd
 *  TcpConnection 设置回调 => Channel => Poller 监听到事件调用回调操作
 */
class TcpConnection:noncopyable,public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,const std::string& name,int sockfd,const InetAddress& localAddr,const InetAddress& peerAddr); 

    ~TcpConnection();

    EventLoop* getLoop() const 
    {
        return this->loop_;
    }
    
    const std::string& name() const
    {
        return this->name_;
    }

    const InetAddress& localAddress() const
    {
        return this->localAddr_;
    }

    const InetAddress& peerAddress() const
    {
        return this->peerAddr_;
    }

    bool connected() const 
    {
        return this->state_ == kConnected;
    }


    void setConnectionCallback(const ConnectionCallback& cb)
    {
        this->connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb)
    {
        this->messageCallback_ = cb;
    }
    
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        this->writeCompleteCallback_ = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb)
    {
        this->highWaterMarkCallback_ = cb;
    }

    void setCloseCallback(const CloseCallback &cb)
    {
        this->closeCallback_ = cb;
    }



    void connectEstablished();

    void connectDestroyed();

    void shutdown();

    void send(const std::string& buf);

private:
    enum StateE
    {
        kDisconnected,kConnecting,kConnected,kDissConnecting
    };

    void setState(StateE state){state_ = state;} 

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void hanldeError();

  
    void sendInLoop(const void* message,size_t len);


    void shutdownInLoop();


    EventLoop *loop_;   // 由subLoop管理TcpConnection
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

};