#include "Server.h"
#include "Logging.h"
#include "Util.h"
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


Server::Server(EventLoop *loop, int threadNum, int port)
:   loop_(loop),//时间循环
    threadNum_(threadNum),
    eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),//初始化线程池
    started_(false),//初始化未启动
    acceptChannel_(new Channel(loop_)),//IO线程
    port_(port),//初始化端口
    listenFd_(socket_bind_listen(port_))//绑定套接字和端口
{
    acceptChannel_->setFd(listenFd_);//让Channel类得知listenfd
    handle_for_sigpipe();//处理服务器主动关闭连接的客户端发送数据
    if (setSocketNonBlocking(listenFd_) < 0)//设置socket为非阻塞IO
    {
        perror("set socket non block failed");
        abort();
    }
}

void Server::start() //启动服务器
{
    eventLoopThreadPool_->start();//开启线程池
    //acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);//设置感兴趣的事件 读和写事件
    acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));
    acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));
    loop_->addToPoller(acceptChannel_, 0);
    started_ = true;
}

void Server::handNewConn()
{
    struct sockaddr_in client_addr;//保存客户端地址
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);//客户端地址长度
    int accept_fd = 0;//传输数据用的句柄
    while((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr, &client_addr_len)) > 0)//建立连接  三次握手
    {
        EventLoop *loop = eventLoopThreadPool_->getNextLoop();
        LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port);//日志信息
        // 限制服务器的最大并发连接数
        if (accept_fd >= MAXFDS)
        {
            close(accept_fd);
            continue;
        }
        // 设为非阻塞模式
        if (setSocketNonBlocking(accept_fd) < 0)//设置套接字为非阻塞模式
        {
            LOG << "Set non block failed!";//日志记录错误信息
            return;
        }

        setSocketNodelay(accept_fd);//关闭nagle算法
        //setSocketNoLinger(accept_fd);

        shared_ptr<ClientRequest> req_info(new ClientRequest(loop, accept_fd));
        req_info->getChannel()->setHolder(req_info);
        loop->queueInLoop(std::bind(&ClientRequest::newEvent, req_info));
    }
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}