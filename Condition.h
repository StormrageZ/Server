#ifndef _CONDITION_H
#define _CONDITION_H

#include "NoCopy.h"
#include "MutexLock.h"
#include <pthread.h>
#include <errno.h>
#include <cstdint>
#include <time.h>
//条件变量的封装
class Conditon: public NoCopy{
public:
    explicit Condition(MutexLock &_mutex):mutex(_mutex){
        pthread_cond_init(&cond, nullptr);//&这里是取地址符号
    }
    ~Condition(){
        pthread_cond_wait(&cond, mutex.GetMutex());
    }

private:
    MutexLock &mutex;
    pthread_cond_t cond;
};

#endif