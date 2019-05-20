#ifndef _ASYNCLOGGING_H
#define _ASYNCLOGGING_H
#include "../CountDownLatch.h"
#include "../Encapsulate/MutexLock.h"
#include "../EventLoop/Thread.h"
#include "LogStream.h"
#include "../Encapsulate/NoCopy.h"
#include <functional>
#include <string>
#include <vector>

class AsyncLogging : NoCopy
{
public:
    AsyncLogging(const std::string basename, int flushInterval = 2);
    ~AsyncLogging()
    {
        if (running_)
            stop();
    }
    void append(const char* logline, int len);

    void start()
    {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop()
    {
        running_ = false;
        cond_.Signal();
        thread_.join();
    }


private:
    void threadFunc();
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
    typedef std::shared_ptr<Buffer> BufferPtr;
    const int flushInterval_;
    bool running_;
    std::string basename_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
    CountDownLatch latch_;
};
#endif