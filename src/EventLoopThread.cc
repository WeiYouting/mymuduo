#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
    const std::string &name  )
    :loop_(nullptr)
    ,exiting_(false)
    ,thread_(std::bind(&EventLoopThread::threadFunc,this),name)
    ,mutex_()
    ,cond_()
    ,callback_(cb)
{

} 

// 析构事件循环
EventLoopThread::~EventLoopThread()
{ 
    this->exiting_ = true;
    if(this->loop_ != nullptr){
        this->loop_->quit();
        this->thread_.join();
    }
}

// 开启事件循环
EventLoop* EventLoopThread::startLoop()
{
    this->thread_.start();

    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(this->mutex_);
        while(this->loop_ == nullptr){
            this->cond_.wait(lock);
        }
        loop = this->loop_;
    }
    return loop;
}

// 启用单独的新线程运行
void EventLoopThread::threadFunc()
{
    // 创建一个独立的EventLoop
    EventLoop loop;

    if(this->callback_){
        this->callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(this->mutex_);
        this->loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(this->mutex_);
    this->loop_ = nullptr;
}
    