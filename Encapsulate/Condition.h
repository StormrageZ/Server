#ifndef _CONDITION_H
#define _CONDITION_H

#include "NoCopy.h"
#include "MutexLock.h"
#include <pthread.h>
#include <errno.h>
#include <cstdint>
#include <time.h>
//条件变量的封装
class Condition: public NoCopy{
public:
    //初始化条件变量
     Condition(MutexLock &_mutex) : mutex(_mutex){
        pthread_cond_init(&cond, nullptr);//&这里是取地址符号
    }
    //注销条件变量，只有在没有线程在等待该cond才可destroy，否则返回EBUSY
    ~Condition(){
       pthread_cond_destroy(&cond);
    }
    //条件等待
    void Wait(){
         pthread_cond_wait(&cond, mutex.GetMutex());
    }
    //激活一个等待该条件的线程
    void Signal(){
        pthread_cond_signal(&cond);
    }
    void SignalAll(){
        pthread_cond_broadcast(&cond);
    }
    //计时等待
    bool TimeWait(int intervals){
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);//获取当前时间
        abstime.tv_sec+=static_cast<time_t>(intervals);
        return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.GetMutex(), &abstime);
    }
private:
    MutexLock &mutex;
    pthread_cond_t cond;
};

#endif