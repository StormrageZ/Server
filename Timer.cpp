#include "Timer.h"
#include <sys/time.h>
#include <unistd.h>
//定时器构造函数
TimerNode::TimerNode(std::shared_ptr<ClientRequest> requestData, int timeout)
:   deleted_(false),
    SPClientRequest(requestData)
{
    struct timeval now;
    gettimeofday(&now, NULL);//获取当前时间
    expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;//精确到毫秒
}

TimerNode::~TimerNode()
{
    if (SPClientRequest)
        SPClientRequest->handleClose();//到期后的处理函数
}

TimerNode::TimerNode(TimerNode &tn):
    SPClientRequest(tn.SPClientRequest)//拷贝构造函数
{ }


void TimerNode::update(int timeout)//更新到期时间
{
    struct timeval now;
    gettimeofday(&now, NULL);
    expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid()//是否到期
{
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    if (temp < expiredTime_)
        return true;
    else
    {
        this->setDeleted();
        return false;
    }
}

void TimerNode::clearReq()
{
    SPClientRequest.reset();//？？？？？
    this->setDeleted();
}


TimerManager::TimerManager()
{ }

TimerManager::~TimerManager()
{ }

void TimerManager::addTimer(std::shared_ptr<ClientRequest> SPClientRequest, int timeout)//添加新的定时器
{
    SPTimerNode new_node(new TimerNode(SPClientRequest, timeout));//new 一个与request相关的定时器
    timerNodeQueue.push(new_node);//往优先队列（最小堆）中添加新的定时器
    SPClientRequest->linkTimer(new_node);//将该定时器与request关联起来
}


/* 处理逻辑是这样的~
因为(1) 优先队列不支持随机访问
(2) 即使支持，随机删除某节点后破坏了堆的结构，需要重新更新堆结构。
所以对于被置为deleted的时间节点，会延迟到它(1)超时 或 (2)它前面的节点都被删除时，它才会被删除。
一个点被置为deleted,它最迟会在TIMER_TIME_OUT时间后被删除。
这样做有两个好处：
(1) 第一个好处是不需要遍历优先队列，省时。
(2) 第二个好处是给超时时间一个容忍的时间，就是设定的超时时间是删除的下限(并不是一到超时时间就立即删除)，如果监听的请求在超时后的下一次请求中又一次出现了，
就不用再重新申请RequestData节点了，这样可以继续重复利用前面的RequestData，减少了一次delete和一次new的时间。
*/

void TimerManager::handleExpiredEvent()
{
    //MutexLockGuard locker(lock);
    while (!timerNodeQueue.empty())//存在定时器
    {
        SPTimerNode ptimer_now = timerNodeQueue.top();//获取最小超时时间的定时器
        if (ptimer_now->isDeleted())
            timerNodeQueue.pop();//标记删除的
        else if (ptimer_now->isValid() == false)//超时的
            timerNodeQueue.pop();
        else
            break;
    }
}