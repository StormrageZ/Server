
#pragma once
#include "../Encapsulate/Condition.h"
#include "../Encapsulate/MutexLock.h"
#include "Thread.h"
#include "../Encapsulate/NoCopy.h"
#include "EventLoop.h"

class EventLoopThread :NoCopy
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();//
    EventLoop *loop_;//该线程的事件循环
    bool exiting_;//是否退出
    Thread thread_;//new thread实例
    MutexLock mutex_;//互斥锁解决线程的同步问题
    Condition cond_;//配合条件变量
};