#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "currentThread.h"

#include <atomic>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>

class Channel;
class Poller;

class EventLoop:noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const{ return pollReturnTime_;}

    // 在当前loop中执行
    void runInLoop(Functor cb);
    // 把cb放入队列中 唤醒loop所在的线程执行cb
    void queueInLoop(Functor cb);

    // 用来唤醒loop所在线程
    void wakeup();
    
    // 调用poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop是否在自己的线程中
    bool isInLoopThread() const{ return threadId_ == CurrentThread::tid();}

private:    
    void handleRead();  // wakeup
    void doPendingFunctors();   // 执行回调

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t threadId_;      // 记录当前loop所在线程的id

    Timestamp pollReturnTime_;  //poller返回发生事件的channel的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;  //当mainLoop获取新用户的channel 通过轮询选择一个subloop 通过该成员唤醒subloop
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    

    std::atomic_bool callingPendingFunctors_;   // 标识当前loop是否有需要执行的回调
    std::vector<Functor> pendingFunctors_;      // 存储loop需要执行的所有回调操作
    std::mutex mutex_;                          // 保证vector容器线程安全
};