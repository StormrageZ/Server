#include "EpollPoller.h"
#include "Util.h"
#include "../Log/Logging.h"
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <queue>
#include <deque>
#include <assert.h>

#include <arpa/inet.h>
#include <iostream>
using namespace std;


const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;


Epoll::Epoll():
    epollFd_(epoll_create1(EPOLL_CLOEXEC)),
    events_(EVENTSNUM)
{   
    cout<<"创建新的epoll_fd: "<<epollFd_<<endl;
    assert(epollFd_ > 0);
}
Epoll::~Epoll()
{ }


// 注册新描述符
void Epoll::epoll_add(std::shared_ptr<Channel> request, int timeout)
{
    int fd = request->getFd();
    if (timeout > 0)
    {
        add_timer(request, timeout);
        fd2http_[fd] = request->getHolder();
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    cout<<"epoll_fd开始监听描述符："<<fd<<endl;
    fd2chan_[fd] = request;
    if(epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll_add error");
        fd2chan_[fd].reset();
    }
}


// 修改描述符状态
void Epoll::epoll_mod(std::shared_ptr<Channel> request, int timeout)
{
    if (timeout > 0)
        add_timer(request, timeout);//添加定时器
    int fd = request->getFd();//accept_FD

        struct epoll_event event;
        event.data.fd = fd;
        event.events = request->getEvents();//感兴趣的事件
        if(epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0)//异常处理
        {
            perror("epoll_mod error");
            fd2chan_[fd].reset();
        }
    
}


// 从epoll中删除描述符
void Epoll::epoll_del(std::shared_ptr<Channel> request)
{
    int fd = request->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0)
    {
        perror("epoll_del error");
    }
    fd2chan_[fd].reset();
    fd2http_[fd].reset();
}




// 返回活跃事件数
std::vector<std::shared_ptr<Channel>> Epoll::poll()
{
    while (true)
    {
        int event_count = epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
        if (event_count < 0)
            perror("epoll wait error");
        std::vector<std::shared_ptr<Channel>> req_data = getEventsRequest(event_count);
        if (req_data.size() > 0)
            return req_data;
    }
}

void Epoll::handleExpired()//处理超时问题
{
    timerManager_.handleExpiredEvent();
}

// 分发处理函数
std::vector<std::shared_ptr<Channel>> Epoll::getEventsRequest(int events_num)
{
    std::vector<std::shared_ptr<Channel>> req_data;
    for(int i = 0; i < events_num; ++i)
    {
        // 获取有事件产生的描述符
        int fd = events_[i].data.fd;

        std::shared_ptr<Channel> cur_req = fd2chan_[fd];
            
        if (cur_req)
        {
            cur_req->setRevents(events_[i].events);
            cur_req->setEvents(0);
            // 加入线程池之前将Timer和request分离
            //cur_req->seperateTimer();
            req_data.push_back(cur_req);
        }
        else
        {
            LOG << "SP cur_req is invalid";
        }
    }
    return req_data;
}

void Epoll::add_timer(std::shared_ptr<Channel> request_data, int timeout)
{
    shared_ptr<ClientRequest> t = request_data->getHolder();
    if (t)
        timerManager_.addTimer(t, timeout);
    else
        LOG << "timer add fail";
}
