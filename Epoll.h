#ifndef _EPOLL_H
#define _EPOLL_H

#include "ParseWeb.h"
#include  <sys/epoll.h>
#include "ThreadPool.h"
const int MAXEVENTS = 2048;
const int SIZE = 1;//Since Linux 2.6.8,the size argument is ignored, but must be greater than zero
int EpollCreate();
int EpollAdd(int epoll_fd, int fd, void *request, __int32_t events);
int EpollMod(int epoll_fd, int fd, void *request, __int32_t events);
int EpollDel(int epoll_fd, int fd, void *request, __int32_t events);
int EpollWait(int epoll_fd, int fd, void *request, __int32_t events);
void HandleEvents(int epoll_fd, int listen_fd, struct epoll_event *events, 
                                        int events_num, char *path, threadpool_t *tp);
#endif
