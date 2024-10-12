#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d mainLoop is null",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop,const InetAddress& listenAddr,const std::string &nameArg,Option option)
    :loop_(CheckLoopNotNull(loop))
    ,ipPort_(listenAddr.toIpPort())
    ,name_(nameArg)
    ,acceptor_(new Acceptor(loop,listenAddr,option == KReusePort))
    ,threadPool_(new EventLoopThreadPool(loop,nameArg))
    ,connectionCallback_()
    ,messageCallback_()          
    ,nextConnId_(1)
    ,started_(0)
{
    // 有新用户连接时 执行TcpServer::newConnection
    this->acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));

}

 TcpServer::~TcpServer()
 {
    for(auto &item:this->connections_){
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        // 销毁连接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }
 }

void TcpServer::newConnection(int sockfd,const InetAddress &peerAddr)
{
    // 选择一个subLoop 管理连接
    EventLoop* ioLoop = this->threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf,sizeof(buf),"-%s#%d",this->ipPort_.c_str(),this->nextConnId_++);
    std::string connName = this->name_ + buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s",
                this->name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());


    sockaddr_in local;
    ::bzero(&local,sizeof(local));
    socklen_t addrlen = sizeof(local);

    if(::getsockname(sockfd,(sockaddr*)&local,&addrlen) < 0){
        LOG_ERROR("socket::getLoalAddr");
    }

    InetAddress localAddr(local);

    // 根据连接成功的sockfd 创建tcpConnection
    TcpConnectionPtr con(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));

    this->connections_[connName] = con;
    con->setConnectionCallback(connectionCallback_);
    con->setMessageCallback(messageCallback_);
    con->setWriteCompleteCallback(writeCompleteCallback_);
    
    con->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));

    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,con));

}

// 设置subLoop个数
void TcpServer::setThreadNum(int numThreads)
{
    this->threadPool_->setThreadNumber(numThreads);
}

// 开启服务器监听
void TcpServer::start()
{
    if(this->started_++ == 0){
        this->threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    this->loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s",this->name_.c_str(),conn->name().c_str());

    connections_.erase(conn->name()); 

    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));

}