#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,const std::string &nameArg)
    :baseLoop_(baseLoop)
    ,name_(nameArg)
    ,started_(false)
    ,numThreads_(0)
    ,next_(0)
{

}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    this->started_ = true;
    for(int i = 0;i < this->numThreads_;++i){
        char buf[this->name_.size() + 32];
        snprintf(buf,sizeof(buf),"%s%d",this->name_.c_str(),i);
        EventLoopThread *t = new EventLoopThread(cb,buf);
        this->threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        this->loops_.push_back(t->startLoop());
    }

    if(this->numThreads_ == 0 && cb){
        cb(baseLoop_);
    }
}

// 如果是多线程环境，默认以轮询的方式分配channel 
EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop* loop = this->baseLoop_;

    if(!this->loops_.empty()){
        loop = this->loops_[this->next_++];
        if(this->next_ >= this->loops_.size()){
            this->next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if(this->loops_.empty()){
        return std::vector<EventLoop*>(1,this->baseLoop_);
    }
    return this->loops_;
}
