#pragma once
#include "NoCopy.h"
#include "ParseWeb.h"
#include <unistd.h>
#include <memory>
#include <queue>
//实现了一个小根堆的定时器及时剔除超时请求，使用了STL的优先队列来管理定时器
//
class ClientRequest;
class TimerNode
{
public:
    TimerNode(std::shared_ptr<ClientRequest> requestData, int timeout);//绑定客户连接和超时时间
    ~TimerNode();
    TimerNode(TimerNode &tn);//拷贝构造函数
    void update(int timeout);//更新超时时间
    bool isValid();//是否到期
    void clearReq();//清楚连接
    void setDeleted() { deleted_ = true; }//删除
    bool isDeleted() const { return deleted_; }
    size_t getExpTime() const { return expiredTime_; } //获取到期时间

private:
    bool deleted_;
    size_t expiredTime_;
    std::shared_ptr<ClientRequest> SPClientRequest;//智能指针RAII思想
};

struct TimerCmp
{
    bool operator()(std::shared_ptr<TimerNode> &a, std::shared_ptr<TimerNode> &b) const
    {
        return a->getExpTime() > b->getExpTime();
    }//重载操作符号（）判断时间大小
};

class TimerManager//时间堆管理定时器
{
public:
    TimerManager();//构造
    ~TimerManager();//析构
    void addTimer(std::shared_ptr<ClientRequest> SPClientRequest, int timeout);//添加新的定时器
    void handleExpiredEvent();//处理到期时间

private:
    typedef std::shared_ptr<TimerNode> SPTimerNode;//定时器智能指针
    std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp> timerNodeQueue;//优先队列管理定时器，底层最小堆
    //MutexLock lock;
};