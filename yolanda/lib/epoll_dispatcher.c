#include  <sys/epoll.h>
#include "event_dispatcher.h"
#include "event_loop.h"
#include "log.h"

#define MAXEVENTS 128

typedef struct {
    int event_count;
    int nfds;   //epoll对象
    int realloc_copy;
    int efd;    //epoll实例
    struct epoll_event *events; //事件集合，epoll_wait的第二个参数
} epoll_dispatcher_data;


static void *epoll_init(struct event_loop *);

static int epoll_add(struct event_loop *, struct channel *channel1);

static int epoll_del(struct event_loop *, struct channel *channel1);

static int epoll_update(struct event_loop *, struct channel *channel1);

static int epoll_dispatch(struct event_loop *, struct timeval *);

static void epoll_clear(struct event_loop *);

/**
    这是什么语法
    这是直接实例化一个结构体吗
    然后它的各个成员都赋值了？
    有点js中定义对象的方式，大括号
*/
const struct event_dispatcher epoll_dispatcher = {
        "epoll",
        epoll_init,
        epoll_add,
        epoll_del,
        epoll_update,
        epoll_dispatch,
        epoll_clear,
};

void *epoll_init(struct event_loop *eventLoop) {
    printf("eventDispatcher结构体的init指针成员指向的是epoll_init()函数.... \n");
    epoll_dispatcher_data *epollDispatcherData = malloc(sizeof(epoll_dispatcher_data));

    epollDispatcherData->event_count = 0;
    epollDispatcherData->nfds = 0;
    epollDispatcherData->realloc_copy = 0;
    epollDispatcherData->efd = 0;

    printf("事件派发器最重要一点是执行epoll_create1()函数，完成epoll对象创建.... \n");
    epollDispatcherData->efd = epoll_create1(0);
    if (epollDispatcherData->efd == -1) {
        error(1, errno, "epoll create failed");
    }
    printf("完成epoll事件结构体的内存申请.... \n");
    epollDispatcherData->events = calloc(MAXEVENTS, sizeof(struct epoll_event));

    return epollDispatcherData;
}

//把channel表示的fd及其感兴趣的事件，加入到epoll，让epoll可以监听
/**
    在epoll_start中调用epoll_wait开始监听套接字
*/
int epoll_add(struct event_loop *eventLoop, struct channel *channel1) {
    printf("eventDispatcher结构体的add指针成员指向的是epoll_add()函数 \n");
    epoll_dispatcher_data *pollDispatcherData = (epoll_dispatcher_data *) eventLoop->event_dispatcher_data;

    int fd = channel1->fd;
    int events = 0;

    //读事件，对应epoll就是EPOLLIN
    //对应poll应该是别的，所以这里是统称事件和具体多路复用的事件转换
    if (channel1->events & EVENT_READ) {
        events = events | EPOLLIN;
    }
    if (channel1->events & EVENT_WRITE) {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
//    event.events = events | EPOLLET;
    //把它加入到epoll中
    printf("最最最终，是调用epoll_ctl，完成channel封装下的fd:%d的添加......\n",fd);
    if (epoll_ctl(pollDispatcherData->efd, EPOLL_CTL_ADD, fd, &event) == -1) {
        error(1, errno, "epoll_ctl add  fd failed");
    }

    return 0;
}

/**
    删除某套接字的监听事件
*/
int epoll_del(struct event_loop *eventLoop, struct channel *channel1) {
    printf("eventDispatcher结构体的del指针成员指向的是epoll_del()函数 \n");

    printf("实际是epoll_del()函数 完成channel的删除\n");
    epoll_dispatcher_data *pollDispatcherData = (epoll_dispatcher_data *) eventLoop->event_dispatcher_data;

    int fd = channel1->fd;

    //删除之前，还得把事件做一次转换，使得epoll函数可以识别
    int events = 0;
    if (channel1->events & EVENT_READ) {
        events = events | EPOLLIN;
    }

    if (channel1->events & EVENT_WRITE) {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
//    event.events = events | EPOLLET;
        printf("最最最终，是调用epoll_ctl，完成channel封装下的fd:%d的删除    \n",fd);
    if (epoll_ctl(pollDispatcherData->efd, EPOLL_CTL_DEL, fd, &event) == -1) {
        error(1, errno, "epoll_ctl delete fd failed");
    }

    return 0;
}

/**
    epoll修改
    应该是某些套接字的事件调整，比如原来监听读事件，调整为write事件而不关心read了
*/
int epoll_update(struct event_loop *eventLoop, struct channel *channel1) {
    epoll_dispatcher_data *pollDispatcherData = (epoll_dispatcher_data *) eventLoop->event_dispatcher_data;

    int fd = channel1->fd;

    int events = 0;
    if (channel1->events & EVENT_READ) {
        events = events | EPOLLIN;
    }

    if (channel1->events & EVENT_WRITE) {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
//    event.events = events | EPOLLET;
    if (epoll_ctl(pollDispatcherData->efd, EPOLL_CTL_MOD, fd, &event) == -1) {
        error(1, errno, "epoll_ctl modify fd failed");
    }

    return 0;
}

/**
这个函数是epollDispatch结构体的指针指向的
*/
int epoll_dispatch(struct event_loop *eventLoop, struct timeval *timeval) {
    printf("eventDispatcher结构体的dispatch指针成员指向的是epoll_dispatch()函数 \n");
    epoll_dispatcher_data *epollDispatcherData = (epoll_dispatcher_data *) eventLoop->event_dispatcher_data;
    int i, n;

    //epoll这一行代码开始监听了哦
    printf( "最终epoll_dispatch()函数中的epoll_wait阻塞\n");
    n = epoll_wait(epollDispatcherData->efd, epollDispatcherData->events, MAXEVENTS, -1);
    printf("epoll_wait函数醒了, 发生了%d个事件，在%s线程 \n", n,eventLoop->thread_name);
    //开始遍历发生的事件数量
    printf("遍历epollDispatcherData的events数组。。。。。。\n");
    for (i = 0; i < n; i++) {
        //有关epoll异常的判断
        if ((epollDispatcherData->events[i].events & EPOLLERR) || (epollDispatcherData->events[i].events & EPOLLHUP)) {
            fprintf(stderr, "epoll error\n");
            printf("哪个fd:%d发生了异常事件, 在%s线程", epollDispatcherData->events[i].data.fd, eventLoop->thread_name);
            close(epollDispatcherData->events[i].data.fd);
            continue;
        }
        //信息来了，需要读
        if (epollDispatcherData->events[i].events & EPOLLIN) {
            printf("哪个fd:%d发生了读事件, 在%s线程", epollDispatcherData->events[i].data.fd, eventLoop->thread_name);
                //注意参数：
                //1eventLoop
                //2哪个套接字
                //3读事件
            channel_event_activate(eventLoop, epollDispatcherData->events[i].data.fd, EVENT_READ);
        }

        //需要写
        if (epollDispatcherData->events[i].events & EPOLLOUT) {
            yolanda_msgx("哪个 channel要写信息 fd==%d, 哪个线程：%s", epollDispatcherData->events[i].data.fd,eventLoop->thread_name);
            //注意参数：
            //1eventLoop
            //2哪个套接字
            //3写事件
            channel_event_activate(eventLoop, epollDispatcherData->events[i].data.fd, EVENT_WRITE);
        }
    }
    printf("遍历epollDispatcherData的events数组结束。。休息3s中。。。。\n");
    sleep(3);

    return 0;
}

/**
    epoll销毁
*/
void epoll_clear(struct event_loop *eventLoop) {
    epoll_dispatcher_data *epollDispatcherData = (epoll_dispatcher_data *) eventLoop->event_dispatcher_data;

    free(epollDispatcherData->events);
    close(epollDispatcherData->efd);
    free(epollDispatcherData);
    eventLoop->event_dispatcher_data = NULL;

    return;
}