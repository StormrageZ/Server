#ifndef _MUTEXLOCK_H
#define _MUTEXLOCK_H

#include "NoCopy.h"
#include <cstdio>
#include <pthread.h>

class MutexLock : public NoCopy{
public:
    MutexLock(){
        pthread_mutex_init(&mutex, nullptr);
    }
    ~MutexLock(){
        pthread_mutex_lock(&mutex);
        pthread_mutex_destroy(&mutex);
    }
    void Lock(){
        pthread_mutex_lock(&mutex);
    }
    void UnLock(){
        pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_t *GetMutex(){
        return &mutex;
    }
private:
    pthread_mutex_t mutex;
private:
    friend class Condition;//友元类，可以直接访问private属性的mutex
};

class MutexLockGuard: public NoCopy{
public:
    explicit MutexLockGuard(MutexLock &_mutex): mutex(_mutex){
        mutex.Lock();
    }
    ~MutexLockGuard(){
        mutex.UnLock();
    }
private:
    MutexLock &mutex;//不可拷贝，但可引用
};
#endif