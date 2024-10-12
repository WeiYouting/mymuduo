#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class EPollPoller:public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;    

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs,ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    // 填写活跃的链接
    void fillActiveChannels(int numEvents,ChannelList *activeChannels) const;
    // 更新channel通道
    void update(int operation,Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};