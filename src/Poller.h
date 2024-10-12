#pragma once

#include "Timestamp.h"
#include "noncopyable.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

class Poller:noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给IO复用保留接口
    virtual Timestamp poll(int timeoutMs,ChannelList *activeChannels) =0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断channel是否在当前poller当中
    bool hasChannel(Channel *channel) const;

    // EventLoop通过该方法获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);


protected:
    // key为sockfd value为sockfd所属的通道
    using ChannelMap = std::unordered_map<int,Channel*>;
    ChannelMap channels_;    
private:
    EventLoop *ownerLoop_;
};