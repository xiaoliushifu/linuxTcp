#include <assert.h>
#include "event_loop_thread.h"
#include "event_loop.h"

/**
    子线程入口函数
*/
void *event_loop_thread_run(void *arg) {
    struct event_loop_thread *eventLoopThread = (struct event_loop_thread *) arg;
    printf("  子线程入口函数开始, %s,当前在%d", eventLoopThread->thread_name,pthread_self());
    printf("子线程先上锁.%s.. \n", eventLoopThread->thread_name);
    pthread_mutex_lock(&eventLoopThread->mutex);


    // 初始化化event loop，之后通知主线程
    yolanda_msgx("子线程内eventLoop初始化, %s", eventLoopThread->thread_name);
    eventLoopThread->eventLoop = event_loop_init_with_name(eventLoopThread->thread_name);

    yolanda_msgx("子线程发信号激活主线程, %s，当前在%d", eventLoopThread->thread_name,pthread_self());
    //printf("子进程的条件变量是%d，互斥变量%d",&eventLoopThread->cond,&eventLoopThread->mutex);
    pthread_cond_signal(&eventLoopThread->cond);

    printf("子线程释放锁..%s，当前在%d \n", eventLoopThread->thread_name,pthread_self());
    pthread_mutex_unlock(&eventLoopThread->mutex);

    //子线程event loop run
    printf("子线程loop_run(), %s，当前在%d\n", eventLoopThread->thread_name,pthread_self());
    event_loop_run(eventLoopThread->eventLoop);
    printf("  子线程入口函数结束, %s,当前在%d\n", eventLoopThread->thread_name,pthread_self());
}

//初始化已经分配内存的event_loop_thread
int event_loop_thread_init(struct event_loop_thread *eventLoopThread, int i) {

    //这两个初始化需要一个指针参数，是出参，即执行init函数后才有值
    pthread_mutex_init(&eventLoopThread->mutex, NULL);
    pthread_cond_init(&eventLoopThread->cond, NULL);

    eventLoopThread->eventLoop = NULL;
    eventLoopThread->thread_count = 0;
    eventLoopThread->thread_tid = 0;

    char *buf = malloc(16);
    sprintf(buf, "Thread-%d\0", i + 1);
    eventLoopThread->thread_name = buf;
    printf("主线程初始化子线程结构：%s... \n",eventLoopThread->thread_name);
    return 0;
}


//由主线程调用，初始化一个子线程，并且让子线程开始运行event_loop
struct event_loop *event_loop_thread_start(struct event_loop_thread *eventLoopThread) {
    printf("主线程创建一个子线程，入口函数是thread_run(),..%s,当前在%d \n",eventLoopThread->thread_name,pthread_self());
    pthread_create(&eventLoopThread->thread_tid, NULL, &event_loop_thread_run, eventLoopThread);

    printf("主线程上互斥锁..%s 当前在%d \n",eventLoopThread->thread_name,pthread_self());
    assert(pthread_mutex_lock(&eventLoopThread->mutex) == 0);

    //从这里，主线程将会挂起，子线程在event_loop_thread_run入口函数开始
    while (eventLoopThread->eventLoop == NULL) {
        printf("主线程主动挂起.等待子进程的eventLoop的创建.%s 当前在%d \n",eventLoopThread->thread_name,pthread_self());
        //printf("主进程的条件变量是%d，互斥变量%d\n",&eventLoopThread->cond,&eventLoopThread->mutex);
        assert(pthread_cond_wait(&eventLoopThread->cond, &eventLoopThread->mutex) == 0);
    }
    printf("主线程释放互斥锁,0..%s 当前在%d \n",eventLoopThread->thread_name,pthread_self());
    assert(pthread_mutex_unlock(&eventLoopThread->mutex) == 0);

    printf("主线程event_loop_thread_start()函数结束, %s  当前在%d \n",eventLoopThread->thread_name,pthread_self());
    return eventLoopThread->eventLoop;
}