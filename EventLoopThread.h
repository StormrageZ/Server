
#pragma once
#include "Condition.h"
#include "MutexLock.h"
#include "Thread.h"
#include "NoCopy.h"
#include "EventLoop.h"

class EventLoopThread :NoCopy
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();
    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};