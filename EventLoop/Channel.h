//A selectable I/O channel.
//This class doesn't own thr file descriptor
//The file descriptor could be a socket
//an eventfd, a timerfd, or a signalfd
//负责一个fd（listen_fd或accepted_fd）的IO事件的分发
#ifndef _CHANNEL_H
#define _CHANNEL_H
#include "../Encapsulate/Timer.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include <functional>
#include <sys/epoll.h>

class Channel
{
public:
    typedef std::function<void()> EventCallBack;
    Channel(EventLoop *loop, int fd);
    ~Channel();
    int getFd();
    void setHolder(std::shared_ptr<ClientRequest> holder)
    {
        holder_ = holder;
    }
    std::shared_ptr<ClientRequest> getHolder()
    {
        std::shared_ptr<ClientRequest> ret(holder_.lock());
        return ret;
    }

    void setReadHandler(EventCallBack &&cb)
    {
        readHandler_ = cb;
    }
    void setWriteHandler(EventCallBack &&cb)
    {
        writeHandler_ = cb;
    }
    void setErrorHandler(EventCallBack &&cb)
    {
        errorHandler_ = cb;
    }
    void setConnHandler(EventCallBack &&cb)
    {
        connHandler_ = cb;
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
    void handleEvents()
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
    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);
    void handleConn();

    void setRevents(int ev)
    {
        revents_ = ev;
    }

    void setEvents(int ev)
    {
        events_ = ev;
    }
    int& getEvents()
    {
        return events_;
    }

    bool EqualAndUpdateLastEvents()
    {
        bool ret = (lastEvents_ == events_);
        lastEvents_ = events_;
        return ret;
    }

    int getLastEvents()
    {
        return lastEvents_;
    }
private:
    
    EventLoop *loop_;
    int  fd_;
    int  events_;
    int revents_;// it's the received event types of epoll used by Epoll
    int lastEvents_;
    std::weak_ptr<ClientRequest> holder_;
    EventCallBack readHandler_;
    EventCallBack writeHandler_;//回调函数
    EventCallBack errorHandler_;
    EventCallBack connHandler_;
};

#endif