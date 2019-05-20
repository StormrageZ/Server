
#pragma once
#include "../EventLoop/EventLoop.h"
#include "../EventLoop/Channel.h"
#include "../EventLoop/EventLoopThreadPool.h"
#include <memory>


class Server
{
public:
    Server(EventLoop *loop, int threadNum, int port);
    ~Server() { }
    EventLoop* getLoop() const { return loop_; }
    void start();
    void handNewConn();
    void handThisConn() { loop_->updatePoller(acceptChannel_); }

private:
    EventLoop *loop_;
    int threadNum_;
    int port_;
    int listenFd_;
    std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
    bool started_;
    std::shared_ptr<Channel> acceptChannel_;
    static const int MAXFDS = 100000;
};