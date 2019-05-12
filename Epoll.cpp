#include "Epoll.h"
#include <errno.h>
#include <stdio.h>
struct epoll_event *Events;
//创建epoll句柄
int EpollCreate(){
    int epoll_fd = epoll_create(SIZE);
    if(epoll_fd == -1)
        return -1;
    Events = new epoll_event[MAXEVENTS];//events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    return epoll_fd;
}
//添加新描述符
int EpollAdd(int epoll_fd, int fd, void *request, __uint32_t events){
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd , &event) == -1){
        perror("EpollAdd Failed!");
        return -1;
    }
    return 0;
}
//修改描述符状态
int EpollMod(int epoll_fd, int fd, void *request, __int32_t events){
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1){
        perror("EpollMod Failed!");
        return -1;
    }
    return 0;
}
//从epoll中删除描述符
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
//返回活跃事件数
int EpollWait(int epoll_fd, struct epoll_event* events, int max_events, int timeout)
{
    int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
    return ret_count;
}
//处理连接事件
void HandleEvents(int epoll_fd, int listen_fd, struct epoll_event *events,    int events_num, char *path, threadpool_t *tp){
    for(int i = 0; i < events_num;++i){
        //TODO
    }
}

