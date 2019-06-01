
#include "EventLoop.h"
#include "../Log/Logging.h"
#include "../Encapsulate/Util.h"
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <iostream>
using namespace std;

__thread EventLoop* t_loopInThisThread = 0;

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);//linux下一个eventfd用于读写
    if (evtfd < 0)
    {
        LOG << "Failed in eventfd";
        abort();
    }
    cout<<"创建eventfd："<<evtfd<<endl;
    return evtfd;
}
//在线程函数中创建eventloop
EventLoop::EventLoop()
:   looping_(false),//是否在循环
    poller_(new Epoll()),//epoll轮询器
    wakeupFd_(createEventfd()),//事件描述符
    quit_(false),//是否关闭
    eventHandling_(false),//事件是否处理中
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),//当前线程ID
    WakeUpChannel(new Channel(this, wakeupFd_))
{   
    cout<<"初始化eventloop ,并创建事件循环所需的epollfd和与主线程通信eventfd"<<endl;
    if (t_loopInThisThread)//one loop in one thread 避免重复创建
    { 
    }
    else
    {
        t_loopInThisThread = this;
    }
    WakeUpChannel->setEvents(EPOLLIN | EPOLLET);
    WakeUpChannel->setReadHandler(bind(&EventLoop::handleRead, this));
    WakeUpChannel->setConnHandler(bind(&EventLoop::handleConn, this));
    poller_->epoll_add(WakeUpChannel, 0);
}

void EventLoop::handleConn()//处理连接
{   
    updatePoller(WakeUpChannel, 0);
}


EventLoop::~EventLoop()
{
    close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::wakeup()
{   
    cout<<"调用wakeup()"<<endl;
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);
    if (n != sizeof one)
    {
        LOG<< "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{   
    cout<<"调用read eventfd"<<endl;
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    WakeUpChannel->setEvents(EPOLLIN | EPOLLET);
}

void EventLoop::runInLoop(Functor&& cb)//运行functor
{
    cout<<"调用runInLoop"<<endl;
    if (isInLoopThread())
        cb();
    else
        queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor&& cb)
{
    cout<<"调用queueInLoop"<<endl;
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }

    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

void EventLoop::loop()//开始loop
{   
    cout<<"开始loop"<<endl;
    assert(!looping_);
    assert(isInLoopThread());
    looping_ = true;
    quit_ = false;//开启状态
    std::vector<std::shared_ptr<Channel>> ret;
    while (!quit_)//循环
    {
        ret.clear();//清空
        ret = poller_->poll();//wait_epoll 获取就绪的事件
        eventHandling_ = true;//标志着事件处理中
        for (auto &it : ret) //C++ 11 for循环标准
            it->handleEvents();//依次处理事件
        eventHandling_ = false;//处理完毕
        doPendingFunctors();//TODO：？
        poller_->handleExpired();//处理超时问题
    }
    looping_ = false;
}

void EventLoop::doPendingFunctors()
{
    cout<<"调用doPendingFunctors"<<endl;
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    callingPendingFunctors_ = false;
}

void EventLoop::quit()//关闭loop
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}