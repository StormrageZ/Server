#ifndef _UTIL_H
#define _UTIL_H

#include <cstdlib>
#include <string>
//封装read write函数 增强鲁棒性
ssize_t readn(int fd, void *buff, size_t n);
ssize_t readn(int fd, std::string &inBuffer, bool &zero);
ssize_t readn(int fd, std::string &inBuffer);
ssize_t writen(int fd, void *buff, size_t n);
ssize_t writen(int fd, std::string &sbuff);

//处理sigpipe信号（当服务器close一个连接时，若client端接着发数据。根据TCP协议的规定，会收到一个RST响应，
//client再往这个服务器发送数据时，系统会发出一个SIGPIPE信号给进程）
void handle_for_sigpipe();
//初始化socket

int setSocketNonBlocking(int fd);//IO复用中，将socket设置为非阻塞模式
void setSocketNodelay(int fd);//关闭nagle算法
void setSocketNoLinger(int fd);//socket选项设置
void shutDownWR(int fd);//关闭连接的一种方式，区别于close
int socket_bind_listen(int port);//socket()->bind()->listen()

#endif