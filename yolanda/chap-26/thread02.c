#include "lib/common.h"

#define  THREAD_NUMBER      4   //线程池的大小
#define  BLOCK_QUEUE_SIZE   3   //连接队列的大小

//声明外部的函数，每个工作线程具体处理连接服务（收发数据）
extern void loop_echo(int);

//初始化线程数据库
typedef struct {
    pthread_t thread_tid;        /* thread ID */
    long thread_count;    /* # connections handled */
} Thread;

Thread *thread_array;

//把连接套接字，放到队列中，让每个子线程到这里取
typedef struct {
    int number;
    int *fd;
    int front;
    int rear;
    //有关线程管理的数据类型，线程锁。线程管理会处理该值吧？
    pthread_mutex_t mutex;
    //有关线程管理的一个数据类型，条件。不深入理解
    pthread_cond_t cond;
} block_queue;

//队列初始化
void block_queue_init(block_queue *blockQueue, int number) {
    blockQueue->number = number;
    blockQueue->fd = calloc(number, sizeof(int));
    blockQueue->front = blockQueue->rear = 0;
    //这俩变量初始化具体原因不懂
    pthread_mutex_init(&blockQueue->mutex, NULL);
    pthread_cond_init(&blockQueue->cond, NULL);
}

//主线程把资源放到队列中
void block_queue_push(block_queue *blockQueue, int fd) {
    //进来后对公共资源先加锁，因为其他线程都在盯着这个公共资源呢
    //主线程往队列里加数据后，子线程再读队列取数据
    pthread_mutex_lock(&blockQueue->mutex);
    //放到尾部，初始就是数组位置0
    blockQueue->fd[blockQueue->rear] = fd;
    //移动尾部指针，这样下一次放置fd时就会放到数组下标1的位置。
    //然后判断是否到达数组末尾，到了就循环，重新从0开始
    //队列长度不够而fd多，就会覆盖之前的fd
    if (++blockQueue->rear == blockQueue->number) {
        blockQueue->rear = 0;
    }
    printf("主线程%d放入fd: %d \n", pthread_self(),fd);
    printf("主线程blockQueue->rear: %d \n", blockQueue->rear);
    printf("主线程blockQueue->front: %d \n", blockQueue->front);
    pthread_cond_signal(&blockQueue->cond);
    //主线程放好后再解锁
    pthread_mutex_unlock(&blockQueue->mutex);
}

//子线程从队列读取fd资源，处理连接
int block_queue_pop(block_queue *blockQueue) {
    //子线程进来后，也是立即加锁
    pthread_mutex_lock(&blockQueue->mutex);
    //初始都是0，说明没有数据，阻塞等待吗？
    while (blockQueue->front == blockQueue->rear){
        //这两个参数，还有这个函数的深入理解：具体啥用不是很清楚
        printf("子线程阻塞到wait这里，%d \n", pthread_self());
        pthread_cond_wait(&blockQueue->cond, &blockQueue->mutex);
    }
    printf("子线程wait后可以去拿资源，%d \n", pthread_self());
    //从队头取数据，最初从0取
    int fd = blockQueue->fd[blockQueue->front];
    //同理，移动下标
    //并判断最大数组时，就重新再来
    if (++blockQueue->front == blockQueue->number) {
        blockQueue->front = 0;
    }
    printf("子线程%d取出fd: %d \n", pthread_self(),fd);
        printf("子线程blockQueue->rear: %d \n", blockQueue->rear);
        printf("子线程blockQueue->front: %d \n", blockQueue->front);
    //取出资源后，解锁队列，使得其他子线程可以进来
    pthread_mutex_unlock(&blockQueue->mutex);
    return fd;
}

//子线程（工作线程）的入口函数
void thread_run(void *arg) {
    //获取自身的线程id
    pthread_t tid = pthread_self();
    pthread_detach(tid);

    block_queue *blockQueue = (block_queue *) arg;
    //这里一直循环，使得该线程处理一次连接后还继续可以复用；
    //好比回到了线程池
    //继续阻塞读取队列，直到新连接又来了，就再去服务
    while (1) {
        //去队列中取资源
        printf("子线程阻塞到这里pop？ tid == %d  \n", tid);
        int fd = block_queue_pop(blockQueue);
        printf("子线程拿到了资源, fd==%d, tid == %d \n", fd, tid);
        //执行业务逻辑
        loop_echo(fd);
    }
}

int main(int c, char **v) {
    int listener_fd = tcp_server_listen(SERV_PORT);

    block_queue blockQueue;
    block_queue_init(&blockQueue, BLOCK_QUEUE_SIZE);

    //初始化一定数量的结构体数组，保存线程id
    thread_array = calloc(THREAD_NUMBER, sizeof(Thread));
    int i;
    //循环创建指定数量的线程，公共参数就是那个队列blockQueue
    for (i = 0; i < THREAD_NUMBER; i++) {
        //第一个参数是出参，就是生成线程之后的线程id,
        //内核生成线程之后，把tid放到第一个参数（指针类型）里，用户态就可以用这个指针类型的参数

        printf("创建一个线程上 %d \n",pthread_self());
        pthread_create(&(thread_array[i].thread_tid), NULL, &thread_run, (void *) &blockQueue);
        printf("创建一个线程下 %d  \n",pthread_self());
    }

    while (1) {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        //阻塞读取连接套接字
        printf("主线程读取连接套接字，会阻塞到accept这里 上 \n");
        int fd = accept(listener_fd, (struct sockaddr *) &ss, &slen);
        printf("主线程会阻塞到accept这里 下 \n");
        if (fd < 0) {
            error(1, errno, "accept failed");
        } else {
            //把描述符资源，放到队列中
            block_queue_push(&blockQueue, fd);
        }
    }

    return 0;
}