#ifndef _EPOLL_H
#define _EPOLL_H

#include "ParseWeb.h"
#include  <sys/epoll>
#include "ThreadPool.h"
const int MAXEVENTS = 2048;
const int SIZE = 1;//Since Linux 2.6.8,the size argument is ignored, but must be greater than zero
int EpollCreate();
int EpollAdd(int epoll_fd, int fd, void *request, int events);
int EpollMod(int epoll_fd, int fd, void *request, int events);
int EpollDel(int epoll_fd, int fd, void *request, int events);
int EpollWait(int epoll_fd, int fd, void *request, int events);

#endif
