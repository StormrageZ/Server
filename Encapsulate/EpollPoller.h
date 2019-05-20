#ifndef _EPOLL_H
#define _EPOLL_H
#include "../EventLoop/Channel.h"
#include "../Server/ParseWeb.h"
#include "../Encapsulate/Timer.h"
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <memory>

class Epoll
{
public:
    Epoll();
    ~Epoll();
    void epoll_add(std::shared_ptr<Channel> request, int timeout);
    void epoll_mod(std::shared_ptr<Channel> request, int timeout);
    void epoll_del(std::shared_ptr<Channel> request);
    std::vector<std::shared_ptr<Channel>> poll();
    std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num);
    void add_timer(std::shared_ptr<Channel> request_data, int timeout);
    int getEpollFd()
    {
        return epollFd_;
    }
    void handleExpired();
private:
    static const int MAXFDS = 100000;
    int epollFd_;
    std::vector<epoll_event> events_;
    std::shared_ptr<Channel> fd2chan_[MAXFDS];
    std::shared_ptr<ClientRequest> fd2http_[MAXFDS];
    TimerManager timerManager_;
};

#endif