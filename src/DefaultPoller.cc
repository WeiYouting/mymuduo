#include "EPollPoller.h"
#include <stdlib.h>


Poller* Poller::newDefaultPoller(EventLoop *loop){
    if(::getenv("MUDUO_USE_POOL")){
        return nullptr;
    }
    return new EPollPoller(loop);
}