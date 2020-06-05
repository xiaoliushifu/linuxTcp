#include "lib/common.h"

//全局变量吧
int another_shared = 0;

//pthread_mutex_t *mutex1;
//    pthread_mutex_t *mutex2;

//    pthread_mutex_init(nutex2, NULL);

//线程入口函数
void * thread_run(void *arg) {
    //公共参数，做类型转换
    int *calculator = (int *) arg;
    printf("一个线程开始执行任务，hello, world, tid == %d \n", pthread_self());
    for (int i = 0; i < 10000; i++) {
        *calculator += 1;
        another_shared += 1;
    }
}

////线程入口函数
//void * thread_run3(void *arg) {
//    //公共参数，做类型转换
//    int *calculator = (int *) arg;
//    //加锁
//    pthread_mutex_lock(mutex1);
//    printf("一个线程tid == %d \n", pthread_self());
//    for (int i = 0; i < 10000; i++) {
//        *calculator += 1;
//        another_shared += 1;
//    }
//    //放锁
//    pthread_mutex_unlock(mutex1);
//}
//
//
//void * thread_run1(void *arg) {
//    //公共参数，做类型转换
//    int *calculator = (int *) arg;
//    for (int i = 0; i < 10; i++) {
//        printf("hello \t");
//    }
//}
//
//void * thread_run2(void *arg) {
//    //公共参数，做类型转换
//    int *calculator = (int *) arg;
//    for (int i = 0; i < 10; i++) {
//        printf("world \t");
//    }
//}

int main(int c, char **v) {
    int calculator;
//    pthread_mutex_init(mutex1,NULL);
//    pthread_mutex_t *mutex1;
//    pthread_mutex_t *mutex2;
//    pthread_mutex_init(nutex1, NULL);
//    pthread_mutex_init(nutex2, NULL);

    pthread_t tid1;
    pthread_t tid2;

    //创建两个线程，每个线程都有入口函数，还有入口函数的参数
    pthread_create(&tid1, NULL, thread_run, &calculator);
    pthread_create(&tid2, NULL, thread_run, &calculator);

    //主线程阻塞到这里，准备回收子线程
    printf("主线程会阻塞等线程tid1 == %d \n", tid1);
    //阻塞函数
    pthread_join(tid1, NULL);
    printf("主线程会阻塞等线程tid2 == %d \n", tid2);
    pthread_join(tid2, NULL);

    printf("calculator is %d \n", calculator);
    printf("another_shared is %d \n", another_shared);
}