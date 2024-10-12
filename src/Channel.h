#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;


class Channel:noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop,int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    // 设置事件回调函数
    void setReadCallback(ReadEventCallback cb) {this->readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb) {this->writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb) {this->closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb) {this->errorCallback_ = std::move(cb);}  

    void tie(const std::shared_ptr<void>&);     


    int fd() const {return this->fd_;}
    void set_revents(int revt) {this->revents_ = revt;}


    // 设置fd对应的事件状态
    void enableReading(){ this->events_ |= kReadEvent;update();}
    void disableReading(){ this->events_ &= ~kReadEvent;update();}
    void enableWriting(){ this->events_ |= kWriteEvent;update();}
    void disableWriting(){ this->events_ &= ~kWriteEvent;update();}
    void disableAll(){ this->events_ = kNoneEvent;update();}

    // 返回fd当前事件状态
    bool isNoneEvent() const { return this->events_ == kNoneEvent;}
    bool isWriting() const { return this->events_ & kWriteEvent;}
    bool isReading() const { return this->events_ & kReadEvent;}

    int index(){ return this->index_;}
    int events(){ return this->events_;}
    void set_index(int idx){ this->index_ = idx;}

    EventLoop* ownerLoop(){ return this->loop_;}
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;       // 事件循环  
    const int fd_;          // fd,poller监听的对象
    int events_;            // fd的感兴趣事件
    int revents_;           // poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_; 

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};