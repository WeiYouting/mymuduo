#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop,int fd)
    :loop_(loop)
    ,fd_(fd)
    ,events_(0)
    ,revents_(0)
    ,index_(-1)
    ,tied_(false)
{

}


Channel::~Channel()
{

}


void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}


/*
 *  当channel所表示的fd的events事件被修改后，update负责在poller里面更改对应fd的epoll_ctl
 */
void Channel::update()
{ 
    // 通过channel所属的EventLoop 调用poller的相应方法注册事件
    // add code
    this->loop_->updateChannel(this);
}

/*
 * 当前channel所属的EventLoop中，删除当前的channel
 */
void Channel::remove()
{   
    // add code
    this->loop_->removeChannel(this);
}


/*
 * fd得到poller通知以后，处理事件
 */
void Channel::handleEvent(Timestamp receiveTime)
{
    if(this->tied_){
        std::shared_ptr<void> guard = this->tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);   
    }
}


/*
 * 根据poller通知channel发生的具体事件，由channel负责调用具体回调函数
 */
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d",this->revents_);

    if((this->revents_ & EPOLLHUP) && !(this->revents_ & EPOLLIN)){
        if(this->closeCallback_){
            this->closeCallback_();
        }
    }
    if((this->revents_ & EPOLLERR)){
        if(this->errorCallback_){
            this->errorCallback_();
        }
    }
    if(this->revents_ & (EPOLLIN | EPOLLPRI)){
        if(this->readCallback_){
            this->readCallback_(receiveTime);
        }
    }
    if(this->revents_ & EPOLLOUT){
        if(this->writeCallback_){
            this->writeCallback_();
        }
    }
}
