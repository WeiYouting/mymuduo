#include <mymuduo/TcpServer.h>
#include <mymuduo/TcpConnection.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>

class EchoServer
{
public:
    EchoServer(EventLoop* loop,const InetAddress& addr,const std::string& name)
        :server_(loop,addr,name)
        ,loop_(loop)
    {
       
        this->server_.setConnectionCallback(std::bind(&EchoServer::onConnection,this,std::placeholders::_1));
        this->server_.setMessageCallback(std::bind(&EchoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
        this->server_.start();
    }
private:
    void onConnection(const TcpConnectionPtr &conn){
        if(conn->connected()){
            LOG_INFO("Connection UP:%s",conn->peerAddress().toIpPort().c_str());
        }else{
            LOG_INFO("Connection DOWN:%s",conn->peerAddress().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn,Buffer* buffer,Timestamp time){
        std::string msg = buffer->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();
    }

    EventLoop* loop_;
    TcpServer server_;
}; 

int main(){
    EventLoop loop;
    InetAddress addr(8080,"192.168.238.133");
    EchoServer server(&loop,addr,"EchoServer-01");
    loop.loop();
}