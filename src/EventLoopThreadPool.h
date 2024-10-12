#pragma once

#include "noncopyable.h"

#include <string>
#include <functional>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop,const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNumber(int numThreads){this->numThreads_ = numThreads;}

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 如果是多线程环境，默认以轮询的方式分配channel 
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started()const{return this->started_;}
    
    const std::string name() const {return this->name_;}

private:
    EventLoop* baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;

};