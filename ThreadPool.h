#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

typedef struct tp_task{
    void (*func)(void*);
    void* arg;
    struct tp_task* next;    // 任务链表（下一节点指针）
}tp_task_t;

typedef struct threadpool{
    pthread_mutex_t lock;    // 互斥锁
    pthread_cond_t cond;    // 条件变量
    pthread_t *threads;    // 线程
    tp_task_t *head;    // 任务链表
    int thread_count;    // 线程数 
    int queue_size;    // 任务链表长
    int shutdown;     // 关机模式
    int started;
}threadpool_t;

typedef enum{
    tp_invalid = -1,
    tp_lock_fail = -2,
    tp_already_shutdown = -3,
    tp_cond_broadcast = -4,
    tp_thread_fail = -5,
}threadpool_error_t;

typedef enum{
    immediate_shutdown = 1,
    graceful_shutdown = 2
}threadpool_sd_t;

threadpool_t* ThreadPool_Init(int thread_count);
int ThreadPoolAdd(threadpool_t* pool, void (*func)(void *), void* arg);
int ThreadPoolDestroy(threadpool_t* pool, int gracegul);
static int ThreadPoolFree(threadpool_t *pool);
static void* ThreadPoolRun(void *arg);
#endif