
#include "Thread.h"
#include <memory>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <assert.h>

#include <iostream>
using namespace std;

namespace CurrentThread
{
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char* t_threadName = "default";
}

pid_t gettid() //获取线程ID
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}
// 为了在线程中保留name,tid这些数据
struct ThreadData //结构体传递多个参数给线程
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;//线程函数
    string name_;//线程名
    pid_t* tid_;//线程ID
    CountDownLatch* latch_;

    ThreadData(const ThreadFunc &func, const string& name, pid_t *tid, CountDownLatch *latch)
    :   func_(func),
        name_(name),
        tid_(tid),
        latch_(latch)
    { }

    void runInThread()
    {
        *tid_ = CurrentThread::tid();
        tid_ = NULL;
        latch_->countDown();//计数器减1, 线程开始执行
        latch_ = NULL;

        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);//设置线程的名字
        func_();//线程函数
        CurrentThread::t_threadName = "finished";//线程名字表示当前功能执行完毕
    }
};

void *startThread(void* obj)
{
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;//防止内存泄漏
    return NULL;
}


Thread::Thread(const ThreadFunc &func, const string &name)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(func),
    name_(name),
    latch_(1)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
        pthread_detach(pthreadId_);
}

void Thread::setDefaultName()
{
    if (name_.empty())
    {
        char buf[32];//设置默认name 为Thread
        snprintf(buf, sizeof buf, "Thread");
        name_ = buf;
    }
}

void Thread::start()
{
    assert(!started_);//防止重复开启
    started_ = true;//更改当前状态为开始
    ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &startThread, data))//创建线程，调用startThread函数，参数为data
    {
        started_ = false; //创建线程失败
        delete data;//删除无用数据，防止内存泄漏
    }
    else
    {
        latch_.wait();//等待开启
        assert(tid_ > 0);
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);//处理结束线程
}