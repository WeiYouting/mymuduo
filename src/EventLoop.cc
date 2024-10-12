#include "EventLoop.h"
#include "Logger.h" 
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd 用于唤醒subReactor
int createEventFd()
{
    int evtfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0){
        LOG_FATAL("eventfd error:%d",errno);
    }
    return evtfd;
}


EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))
    ,wakeupFd_(createEventFd())
    ,wakeupChannel_(new Channel(this,wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d",this,threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d",t_loopInThisThread,threadId_);
    }else{
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型及发生事件的回调函数
    this->wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    // 每一个EventLoop都监听wakeupChannel的EPOLLIN读事件
    wakeupChannel_->enableReading();
}


EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(this->wakeupFd_);
    t_loopInThisThread = nullptr;
}


// 开启事件循环
void EventLoop::loop()
{
    this->looping_ = true;
    this->quit_ = false;
    
    LOG_INFO("EventLoop %p start loopint",this);
    
    while(!quit_){
        this->activeChannels_.clear();
        this->pollReturnTime_ = this->poller_->poll(kPollTimeMs,&this->activeChannels_);
        for(Channel *channel:this->activeChannels_){
            // poller监听哪些channel发生事件 上报给EventLoop 通知channel处理对应的事件
            channel->handleEvent(this->pollReturnTime_);
        }
        // 执行当前EventLoop需要处理的回调操作
        // mainloop注册回调  wakeup subloop后，执行该方法
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping",this);
    this->looping_ = false;
}


// 退出事件循环
void EventLoop::quit()
{
    this->quit_ = true;

    // 退出事件循环 如果在其他线程调用quit 则唤醒
    if(!isInLoopThread()){
        wakeup();
    }
}



void EventLoop::runInLoop(Functor cb)
{
    // 在当前loop中执行
    if(isInLoopThread()){
        cb();
    }else{
       queueInLoop(cb);  // 不在当前线程中执行cb  唤醒loop所在线程
    }
}


// 把cb放入队列中 唤醒loop所在的线程执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(this->mutex_);
        this->pendingFunctors_.emplace_back(cb);
    }

    // loop不属于当前线程 或当前loop正在执行回调
    if(!isInLoopThread() || this->callingPendingFunctors_){
        wakeup();
    }
}


void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(this->wakeupFd_,&one,sizeof(one));
    if(n != sizeof(one)){
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8",n); 
    }
}

// 用来唤醒loop所在线程
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(this->wakeupFd_,&one,sizeof(one));
    if(n != sizeof(one)){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8",n);
    }
}


// 调用poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    this->poller_->updateChannel(channel);
}


void EventLoop::removeChannel(Channel *channel)
{
    this->poller_->removeChannel(channel);
}


bool EventLoop::hasChannel(Channel *channel)
{
    return this->poller_->hasChannel(channel);
}


void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    this->callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(this->mutex_);
        functors.swap(this->pendingFunctors_);
    }
    
    for(const Functor &functor:functors){
        //执行当前loop需要执行的回调操作
        functor();
    }

    this->callingPendingFunctors_ = false;
}