#include "Epoll.h"
#include <errno.h>
struct epoll_event *Events;

int EpollCreate(){
    int epoll_fd = epoll_create(SIZE);
    if(epoll_fd == -1)
        return -1;
    Events = new epoll_event[MAXEVENTS];//events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    return epoll_fd;
}

int EpollAdd(int epoll_fd, int fd, void *request, __uint32_t events){
    struct epoll_event event;
    event.data.prt = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd , &event) == -1){
        perror("EpollAdd Failed!");
        return -1;
    }
    return 0;
}

int EpollMod(int epoll_fd, int fd, void *request, __uint32_t events){
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_ctl_MOD, fd, &events) == -1){
        perror("EpollMod Failed!");
        return -1;
    }
    return 0;
}

int EpollDel(int epoll_fd, int fd, void *request, __uint32_t events){
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event) < 0)
    {
        perror("EpollDel Failed!");
        return -1;
    } 
    return 0;
}

int EpollWait(int epoll_fd, struct epoll_event* events, int max_events, int timeout)
{
    int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
    return ret_count;
}
