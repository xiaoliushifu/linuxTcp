#include <assert.h>
#include "utils.h"
#include "thread_pool.h"

struct thread_pool *thread_pool_new(struct event_loop *mainLoop, int threadNumber) {

    struct thread_pool *threadPool = malloc(sizeof(struct thread_pool));
    threadPool->mainLoop = mainLoop;
    threadPool->position = 0;
    threadPool->thread_number = threadNumber;
    threadPool->started = 0;
    threadPool->eventLoopThreads = NULL;
    return threadPool;
}

//一定是main thread发起
void thread_pool_start(struct thread_pool *threadPool) {
    //assert(exp)函数是判断参数表达式为真就往下进行，否则就退出，类似于：
    /**
    if(!exp) {
        exit();
    }
    .....
    */
    //这里是开启线程池，所以线程池应该还没开启，才能被开启
    assert(!threadPool->started);
    //判断mainLoop这个结构体成员记录的线程id，是不是当前线程id
    //不是就记录日志报错，然后退出
    //确保当前线程是主线程，不是子线程
    assertInSameThread(threadPool->mainLoop);
    //置为1，后面就进不来了，也就是进不到这里了
    threadPool->started = 1;

    //空指针？？？
    void *tmp;

    //如果真的小于0，那就说明无需开线程池，直接走人
    if (threadPool->thread_number <= 0) {
        return;
    }
    //可以一次性申请整块数组的内存，在此说明数组的内存地址都是连续的
    //申请n个大小为event_loop_thread的连续内存空间
    threadPool->eventLoopThreads = malloc(threadPool->thread_number * sizeof(struct event_loop_thread));
    for (int i = 0; i < threadPool->thread_number; ++i) {
        //这俩函数，还是封装，还不是之前见过的c原生线程生成函数
        event_loop_thread_init(&threadPool->eventLoopThreads[i], i);
        event_loop_thread_start(&threadPool->eventLoopThreads[i]);
    }
}

/**
    主线程从线程池中挑选一个子线程
    函数开头有一些检测：
        比如线程池是否启动了
        当前线程是不是主线程
        我怀疑assert到底会直接退出当前所在函数还是像抛异常那样导致程序中断？后来测试是抛异常退出
*/
struct event_loop *thread_pool_get_loop(struct thread_pool *threadPool) {
    //首先线程池得启动（配置0说明没启动）
    assert(threadPool->started);
    //当前线程必须是主线程才行
    assertInSameThread(threadPool->mainLoop);

    //优先选择当前主线程
    struct event_loop *selected = threadPool->mainLoop;

    //从线程池中按照顺序挑选出一个线程
    if (threadPool->thread_number > 0) {
        //如果线程池可以挑选，则很可能覆盖主线程
        printf("线程池中的eventLoop,可以使用。。。。。。\n");
        selected = threadPool->eventLoopThreads[threadPool->position].eventLoop;
        //线程轮流着来
        if (++threadPool->position >= threadPool->thread_number) {
            printf("线程池满，重置线程池下标为0... \n");
            threadPool->position = 0;
        }
    }

    return selected;
}
