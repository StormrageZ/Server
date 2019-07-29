#include "Channel.h"
#include "../Encapsulate/Util.h"
#include "../Encapsulate/EpollPoller.h"
#include "EventLoop.h"
#include <unistd.h>
#include <queue>
#include <cstdlib>
#include <iostream>
using namespace std;
Channel::Channel(EventLoop *loop, int fd):
    loop_(loop),
    fd_(fd), 
    events_(0),
    lastEvents_(0)
{ }


int Channel::getFd()
{
    return fd_;
}
 void Channel::setHolder(std::shared_ptr<ClientRequest> holder)
{
    holder_ = holder;
}
    std::shared_ptr<ClientRequest> Channel::getHolder()
{
    std::shared_ptr<ClientRequest> ret(holder_.lock());
    return ret;
}

void Channel::setReadHandler(EventCallBack &&cb)
{
    readHandler_ = cb;
}
void Channel::setWriteHandler(EventCallBack &&cb)
{
    writeHandler_ = cb;
}
void Channel::setErrorHandler(EventCallBack &&cb)
{
    errorHandler_ = cb;
}
    void Channel::setConnHandler(EventCallBack &&cb)
{
    connHandler_ = cb;
}
void Channel::handleRead()
{
    if (readHandler_)
    {
        readHandler_();
    }
}

void Channel::handleWrite()
{
    if (writeHandler_)
    {
        writeHandler_();
    }
}

void Channel::handleConn()
{
    if (connHandler_)
    {
        connHandler_();
    }
}
 /*
    EPOLLIN ，读事件
    EPOLLPRI，读事件，但表示紧急数据，例如tcp socket的带外数据
    POLLRDNORM , 读事件，表示有普通数据可读　　　
    POLLRDBAND ,　读事件，表示有优先数据可读　　　　
    EPOLLOUT，写事件
    POLLWRNORM , 写事件，表示有普通数据可写
    POLLWRBAND ,　写事件，表示有优先数据可写　　　   　　　　
    EPOLLRDHUP (since LinuE 2.6.17)，Stream socket的一端关闭了连接（注意是stream socket，我们知道还有raw socket,dgram socket），或者是写端关闭了连接，如果要使用这个事件，必须定义_GNU_SOURCE 宏。这个事件可以用来判断链路是否发生异常（当然更通用的方法是使用心跳机制）。要使用这个事件，得这样包含头文件：
    　　#define _GNU_SOURCE
      　　#include <poll.h>
    EPOLLERR，仅用于内核设置传出参数revents，表示设备发生错误
    EPOLLHUP，仅用于内核设置传出参数revents，表示设备被挂起，如果poll监听的fd是socket，表示这个socket并没有在网络上建立连接，比如说只调用了socket()函数，但是没有进行connect。
    EPOLLNVAL，仅用于内核设置传出参数revents，表示非法请求文件描述符fd没有打开
    */
void Channel::handleEvents()
{
    events_ = 0;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        events_ = 0;//无效事件
        return;
    }
    if (revents_ & EPOLLERR)
    {
        if (errorHandler_) errorHandler_();//设备错误
        events_ = 0;
        return;
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))//可读事件
    {
        handleRead();
    }
    if (revents_ & EPOLLOUT)//可写事件
    {
        handleWrite();
    }
    handleConn();
}
void Channel::setRevents(int ev)
{
    revents_ = ev;
}

void Channel::setEvents(int ev)
{
    events_ = ev;
}
int& Channel::getEvents()
{
    return events_;
}

