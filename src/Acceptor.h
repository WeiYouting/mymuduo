#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor:noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb){this->newConnectionCallback_ = std::move(cb);}
    
    bool listenning() const {return this->listenning_;}

    void listen();

private:
    void handleRead();

    EventLoop *loop_;   // mainLoop  用户自定义的loop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};