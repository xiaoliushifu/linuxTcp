/*
    一些练习的代码
    一个多线程争抢执行的场景
    如何编译：
        gcc t.c -c -o t.o
        gcc t.o -o t -lpthread
        最后的 -lpthread关键
*/

#include<pthread.h>
#include<stdio.h>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

void * func(void * arg)
{
        //下面这一行啥意思呢？
        int threadno = *(int*)arg;
        int i=0;
        for(;i<10;i++)
        {
                printf("%d thread%d \n",threadno,i);
                sleep(1);
        }

        return NULL;

}
int main()
{

        pthread_t t1,t2;

        int i1=1,i2=2;
        pthread_create(&t1,NULL,func,&i1);
        pthread_create(&t2,NULL,func,&i2);

        pthread_join(t1,NULL);
        pthread_join(t2,NULL);

        printf("主线程退出\n");
        return EXIT_SUCCESS;

}