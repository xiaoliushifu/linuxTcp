/*

    线程等待和被一个线程唤醒
    如何编译：
        gcc cond.c -c -o cond.o
        gcc cond.o -o cond -lpthread
        最后的 -lpthread关键


   下面来讲一下：pthread_cond_wait和pthread_cond_singal是怎样配对使用的：

        等待线程：
            pthread_cond_wait前要先加锁
            pthread_cond_wait内部会解锁，然后等待条件变量被其它线程激活
            pthread_cond_wait被激活后会再自动加锁
        激活线程：
            加锁（和等待线程用同一个锁）
            pthread_cond_signal发送信号
            解锁
        激活线程的上面三个操作在运行时间上都在等待线程的pthread_cond_wait函数内部。
        也就是说，直到激活线程解锁，等待线程才会从wait中返回

        同样一段多线程的代码，在多核和单核cpu的环境下执行效果有差别
        比如在虚拟机单核场景下：很难看到多线程（多进程）争抢的现象；
        而在mac（2核）环境下，很容易看到多线程争抢资源的线程；

        虚拟机单核场景下，虽然多线程代码执行，但是cpu分给每个线程的时间片不是绝对相等的；
        有可能这个线程的时间片长一些，就多执行一段时间，经常看到一个线程执行很长时间（比如循环）后才给其他线程执行
        ，好像cpu看着这个线程能执行就多给他点时间片，尽量不打断这个线程似的。
*/

#include<stdio.h>
#include<sys/types.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

int count = 0;


void *decrement(void *arg) {
    printf("11 derement.\n");
    pthread_mutex_lock(&mutex);//wait前加锁
    if (count == 0) {
            printf("11 阻塞到wait吗.\n");
            pthread_cond_wait(&cond, &mutex);//需要传递锁，因为内核会释放锁
            printf("11 wait下一行吗.\n");
    }
    int i=0;
    while(i < 10) {
            printf("11111.\n");
            i++;
        }
    count--;
    printf("11----decrement:%d.\n", count);
    printf("11 out decrement.\n");
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *increment(void *arg) {
    printf("22 increment.\n");
    pthread_mutex_lock(&mutex);
    count++;
    printf("22----increment:%d.\n", count);
    if (count != 0) {
            pthread_cond_signal(&cond);//无需用锁
            printf("22  signal之后.\n");
            sleep(15);
    }
    printf("22 out increment.\n");
    pthread_mutex_unlock(&mutex);
    int i=0;
    while(i < 10) {
        printf("22222.\n");
//        sleep(1);
        i++;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t tid_in, tid_de;
    pthread_create(&tid_de, NULL, (void*)decrement, NULL);
    sleep(2);
    pthread_create(&tid_in, NULL, (void*)increment, NULL);
    sleep(5);

    pthread_join(tid_de, NULL);
    pthread_join(tid_in, NULL);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}