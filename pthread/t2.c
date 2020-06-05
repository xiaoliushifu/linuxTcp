/*

    一个多线程争抢执行的场景，使用锁后使得某些代码块同步（串行化）
    锁对各个线程来说是全局变量
    如何编译：
        gcc t2.c -c -o t2.o
        gcc t2.o -o t2 -lpthread
        最后的 -lpthread关键
*/

#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

//初始化一个快速互斥锁
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void * func(void * arg)
{
        //首先获取锁
        printf("锁1 %p \n",&mutex);
        pthread_mutex_lock(&mutex);//对mutex加锁，其他线程进入后将会挂起，知道这个锁被解锁

        int threadno =*(int*)arg;
        int i=0;
        for(;i<10;i++)
        {
                printf("%d thread%d \n",threadno,i);
                sleep(1);
        }
        pthread_mutex_unlock(&mutex);

        return NULL;

}
//void * func2(void * arg)
//{
//        printf("锁2 %p \n",&mutex2);
//        //首先获取锁
//        pthread_mutex_lock(&mutex2);//对mutex加锁，其他线程进入后将会挂起，知道这个锁被解锁
//
//        int threadno =*(int*)arg;
//        int i=0;
//        for(;i<10;i++)
//        {
//                printf("%d thread%d \n",threadno,i);
//                sleep(1);
//        }
//        pthread_mutex_unlock(&mutex2);
//
//        return NULL;
//
//}
int main()
{

        pthread_t t1,t2;

        int i1=1,i2=2;
        pthread_create(&t1,NULL,func,&i1);
        pthread_create(&t2,NULL,func,&i2);

        //pthread_join，阻塞，也就是说主线程是挂起了，
        //但是如果主线程不挂起而直接执行，
        //如果主线程提前退出，那么子线程也会被迫退出
        //虽然线程之间是同级的，但是主线程还是有点特权的。
        printf("阻塞等待t1线程退出\n");
        pthread_join(t1,NULL);
        printf("t1线程退出\n");
        printf("阻塞等待t2线程退出\n");
        pthread_join(t2,NULL);
        printf("t2线程退出\n");

        printf("主线程退出\n");
        return EXIT_SUCCESS;

}