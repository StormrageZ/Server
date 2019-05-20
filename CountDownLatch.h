
#pragma once
#include "Encapsulate/Condition.h"
#include "Encapsulate/MutexLock.h"
#include "Encapsulate/NoCopy.h"

// CountDownLatch的主要作用是确保Thread中传进去的func真的启动了以后
// 外层的start才返回
class CountDownLatch : NoCopy
{
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();

private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};