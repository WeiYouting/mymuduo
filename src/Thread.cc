#include "Thread.h"

#include "currentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func,const std::string &name)
    :started_(false)
    ,joined_(false)
    ,tid_(0)
    ,func_(std::move(func))
    ,name_(name)
{
    setDefaultName();

}

Thread::~Thread()
{
    if(this->started_ && !this->joined_){
        this->thread_->detach();

    }
}


void Thread::start()
{
    this->started_ = true;

    sem_t sem;
    sem_init(&sem,false,0);

    this->thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程tid
        this->tid_ = CurrentThread::tid();
        sem_post(&sem);
        this->func_();

    }));

    sem_wait(&sem);

}


void Thread::join()
{
    this->joined_ = true;
    this->thread_->join();
}



void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(this->name_.empty()){
        char buf[32] = {0};
        snprintf(buf,sizeof(buf),"Thraed%d",num);
        this->name_ = buf;
    }
}
