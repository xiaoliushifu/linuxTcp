#include <assert.h>
#include "event_loop.h"
#include "common.h"
#include "log.h"
#include "event_dispatcher.h"
#include "channel.h"
#include "utils.h"

/**
    在注册完一次channel后（加入到pending队列后）
    调用该方法，要干啥呢？
*/
// in the i/o thread
int event_loop_handle_pending_channel(struct event_loop *eventLoop) {
    //get the lock
    printf("pending链表上锁。 %s... \n",eventLoop->thread_name);
    pthread_mutex_lock(&eventLoop->mutex);
    //设置为1，不能再进来了
    eventLoop->is_handle_pending = 1;

    //从head开始,channelElement包裹这channel
    struct channel_element *channelElement = eventLoop->pending_head;
    //遍历channelElement链表
    printf("遍历pending链表,把channelElement都处理完。。。。。。 %s... \n",eventLoop->thread_name);
    while (channelElement != NULL) {
        //save into event_map
        struct channel *channel = channelElement->channel;
        int fd = channel->fd;
        //1，新来的，需要把channel通过fd和channelMap建立关系
        if (channelElement->type == 1) {
            printf("channel-fd:%d的type=1，add..%s. \n",fd,eventLoop->thread_name);
            event_loop_handle_pending_add(eventLoop, fd, channel);
            //表示删除
        } else if (channelElement->type == 2) {
            printf("channel-fd:%d的type=2，remove...%s. \n",fd,eventLoop->thread_name);
            event_loop_handle_pending_remove(eventLoop, fd, channel);
        } else if (channelElement->type == 3) {
            //更新
            printf("channel-fd:%d的type=3，update...%s. \n",fd,eventLoop->thread_name);
            event_loop_handle_pending_update(eventLoop, fd, channel);
        }
        channelElement = channelElement->next;
    }
    printf("pending链表处理完了。。。。。。 %s... \n",eventLoop->thread_name);
    //完事再初始化为0
    eventLoop->pending_head = eventLoop->pending_tail = NULL;
    eventLoop->is_handle_pending = 0;

    //release the lock
    printf("pending链表释放锁。 %s... \n",eventLoop->thread_name);
    pthread_mutex_unlock(&eventLoop->mutex);

    return 0;
}

/**
    内部方法
    真正完成把channel1封装为channel_element，注册到eventLoop中
    通俗来讲，所谓注册到eventLoop，就是放到eventLoop维护的一个队列上而已
    。。。。
    后续eventLoop启动时，就会把队列里的channel一个个绑定到epoll_wait上
    后续事件派发器会从这个队列中遍历
*/
void event_loop_channel_buffer_nolock(struct event_loop *eventLoop, int fd, struct channel *channel1, int type) {
    //add channel into the pending list
    printf("把channel1封装为channelElement放到eventLoop的pending队列中 %s... \n",eventLoop->thread_name);
    struct channel_element *channelElement = malloc(sizeof(struct channel_element));
    channelElement->channel = channel1;
    channelElement->type = type;
    channelElement->next = NULL;
    if (eventLoop->pending_head == NULL) {
        printf("第一个channelElement，放到eventLoop的pending队首和队尾 %s... \n",eventLoop->thread_name);
        eventLoop->pending_head = eventLoop->pending_tail = channelElement;
    } else {
        //后续的channel
        printf("不是第一个channelElement，放到eventLoop的pending队尾 %s... \n",eventLoop->thread_name);
        eventLoop->pending_tail->next = channelElement;
        eventLoop->pending_tail = channelElement;
    }
}

/**
    完成channel1注册到eventLoop的功能
    1。一般初始化时，监听套接字封装的channel会先注册
    2。 后续新客户端连接到来时，accept返回的fd也会封装为channel然后注册
   所以，目前上述两个场景会调用该方法
   3。连接发生数据时呢？
*/
int event_loop_do_channel_event(struct event_loop *eventLoop, int fd, struct channel *channel1, int type) {
    //get the lock
    //首先获得锁
    printf("执行event_loop_do_channel_event函数，注册channel  eLName%s 当前在%d\n",eventLoop->thread_name,pthread_self());
    pthread_mutex_lock(&eventLoop->mutex);
    //这个is_handle_pending确保等于0，否则退出程序
    assert(eventLoop->is_handle_pending == 0);

    //实例化channel_element，并放到eventLoop的成员head,tail这些
    printf("调用event_loop_channel_buffer_nolock()方法，fd:%d...%s \n",channel1->fd,eventLoop->thread_name);
    event_loop_channel_buffer_nolock(eventLoop, fd, channel1, type);
    //release the lock
    //释放锁
    pthread_mutex_unlock(&eventLoop->mutex);


    if (!isInSameThread(eventLoop)) {
        printf("调用event_loop_wakeup唤醒函数，fd:%d.. eLname%s. 当前在%d\n",channel1->fd,eventLoop->thread_name,pthread_self());
        event_loop_wakeup(eventLoop);
    } else {
        printf("channel_event内部调用event_loop_handle_pending()函数，fd:%d...%s \n",channel1->fd,eventLoop->thread_name);
        event_loop_handle_pending_channel(eventLoop);
    }
printf("执行event_loop_do_channel_event函数结束 eLName%s 当前在%d\n",eventLoop->thread_name,pthread_self());
    return 0;

}

/**
    channel注册到eventLoop上
*/
int event_loop_add_channel_event(struct event_loop *eventLoop, int fd, struct channel *channel1) {
    printf("执行event_loop_add_channel_event函数，第四个参数是1，注册channel,fd:%d, %s \n",channel1->fd,eventLoop->thread_name);
    return event_loop_do_channel_event(eventLoop, fd, channel1, 1);
}

int event_loop_remove_channel_event(struct event_loop *eventLoop, int fd, struct channel *channel1) {
    printf("执行event_loop_add_channel_event函数，第四个参数是2, \n");
    return event_loop_do_channel_event(eventLoop, fd, channel1, 2);
}

int event_loop_update_channel_event(struct event_loop *eventLoop, int fd, struct channel *channel1) {
    return event_loop_do_channel_event(eventLoop, fd, channel1, 3);
}

// in the i/o thread
int event_loop_handle_pending_add(struct event_loop *eventLoop, int fd, struct channel *channel) {
    printf("执行event_loop_handle_pending_add()函数... %s... \n",eventLoop->thread_name);
    //yolanda_msgx("添加一个channel channel fd == %d, %s", fd, eventLoop->thread_name);
    struct channel_map *map = eventLoop->channelMap;

    if (fd < 0)
        return 0;

    if (fd >= map->nentries) {
            printf("重置map空间... %s... \n",eventLoop->thread_name);
        if (map_make_space(map, fd, sizeof(struct channel *)) == -1)
            return (-1);
    }

    //entries中不存在这个套接字，那就把fd对应的channel放到里面
    //注意：就是这里把map->fd->channel 三者建立了关系
    if ((map)->entries[fd] == NULL) {
        printf("map和channel建立关系，map->entries[fd:%d] = channel ..%s. \n",fd,eventLoop->thread_name);
        map->entries[fd] = channel;
        //add channel
        struct event_dispatcher *eventDispatcher = eventLoop->eventDispatcher;
        //调用add方法
        printf("执行eventDispatcher的add函数 ..%s... \n",eventLoop->thread_name);
        eventDispatcher->add(eventLoop, channel);
        return 1;
    }

    return 0;
}

/**

    完成channel从eventLoop的删除
*/
// in the i/o thread
int event_loop_handle_pending_remove(struct event_loop *eventLoop, int fd, struct channel *channel1) {
    struct channel_map *map = eventLoop->channelMap;
    assert(fd == channel1->fd);

    if (fd < 0)
        return 0;

    if (fd >= map->nentries)
        return (-1);


    struct channel *channel2 = map->entries[fd];

    //update dispatcher(multi-thread)here
    struct event_dispatcher *eventDispatcher = eventLoop->eventDispatcher;

    int retval = 0;
    //调用事件派发器的方法del，删除
    printf("最终交给eventDispatcher的del方法去删除channel... \n",fd);
    if (eventDispatcher->del(eventLoop, channel2) == -1) {
        retval = -1;
    } else {
        retval = 1;
    }
    //map中的套接字下标，置空
    printf("map->entries数组的fd:%d下标对应的channel,置空... \n",fd);
    map->entries[fd] = NULL;
    return retval;
}

// in the i/o thread
int event_loop_handle_pending_update(struct event_loop *eventLoop, int fd, struct channel *channel) {
    yolanda_msgx("update channel fd == %d, %s", fd, eventLoop->thread_name);
    struct channel_map *map = eventLoop->channelMap;

    if (fd < 0)
        return 0;

    if ((map)->entries[fd] == NULL) {
        return (-1);
    }

    //update channel
    struct event_dispatcher *eventDispatcher = eventLoop->eventDispatcher;
    eventDispatcher->update(eventLoop, channel);
}
/**
    事件激活的回调
       1 event_loop
       2哪个套接字的事件
       3revents，返回的事件，从epoll_wait的出参返回的事件，通过它知道到底发送了什么事件（读或者写）
*/
int channel_event_activate(struct event_loop *eventLoop, int fd, int revents) {
    printf("channel_event_activate()方法执行.. \n");
    struct channel_map *map = eventLoop->channelMap;
    printf("连接fd:%d, 有事件revents=%d 发生，当前工作线程是：%s\n", fd, revents, eventLoop->thread_name);

    if (fd < 0)
        return 0;

    if (fd >= map->nentries)return (-1);
    //以fd为下标，取出channelMap中的channel,可见channelMap和channel是如何映射关系
    //因为epoll_wait返回哪个套接字哪个事件，不会直接给channel,这里通过fd和map的关系，就能找到channel了

    printf("通过map快速把fd映射到具体的channel.. \n");
    struct channel *channel = map->entries[fd];
    //多验证下，epoll_wait返回的fd和channelMap关系给的fd，理论上应该是一样的
    assert(fd == channel->fd);

    if (revents & (EVENT_READ)) {
        //读事件，自然是调用channel的read指针函数
        //但是：我们知道总共有两类套接字：
                // 1监听套接字的read回调，肯定是accept操作
                // 2连接套接字的read，肯定就是读取业务数据了
          printf("channel-fd:%d发生读事件..，执行它当初注册时的回调eventReadCallback指针。。。。。。 \n",fd);
        if (channel->eventReadCallback) channel->eventReadCallback(channel->data);
    }
    if (revents & (EVENT_WRITE)) {
        printf("channel-fd:%d发生写事件..，执行它当初注册时的回调eventWriteCallback指针。。。。。。 \n",fd);
        if (channel->eventWriteCallback) channel->eventWriteCallback(channel->data);
    }
    printf("channel_event_activate()方法执行结束..当前在%d \n",pthread_self());
    return 0;

}

/**
    这个操作是啥意思？？？

*/
void event_loop_wakeup(struct event_loop *eventLoop) {

    printf("执行event_loop_wakeup()函数...elName%s, 当前在%d \n",eventLoop->thread_name,pthread_self());
    char one = 'a';
    //往套接字里写一个字符a
    printf("往参数eventLoop里的socketPair[0]发一个字符...elName%s, 当前在%d \n",eventLoop->thread_name,pthread_self());
    ssize_t n = write(eventLoop->socketPair[0], &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERR("wakeup event loop thread failed");
    }
    printf("event_loop_wakeup()函数结束...elName%s, 当前在%d \n",eventLoop->thread_name,pthread_self());
}

/**

*/
int handleWakeup(void *data) {
    printf("handleWakeup()函数执行......\n");
    struct event_loop *eventLoop = (struct event_loop *) data;
    char one;
    ssize_t n = read(eventLoop->socketPair[1], &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERR("handleWakeup  failed");
    }
    printf("socketPair[1]套接字读取, 在%s线程\n", eventLoop->thread_name);
}

struct event_loop *event_loop_init() {
    return event_loop_init_with_name(NULL);
}

struct event_loop *event_loop_init_with_name(char *thread_name) {
    printf("eventLoop开始初始化。。。。。。 工作线程是：%s\n", thread_name);
    struct event_loop *eventLoop = malloc(sizeof(struct event_loop));
    pthread_mutex_init(&eventLoop->mutex, NULL);
    pthread_cond_init(&eventLoop->cond, NULL);

    //把名字加上
    if (thread_name != NULL) {
        eventLoop->thread_name = thread_name;
    } else {
        eventLoop->thread_name = "main thread";
    }

    eventLoop->quit = 0;
    printf("成员channelMap初始化... 当前工作线程是：%s\n", eventLoop->thread_name);
    eventLoop->channelMap = malloc(sizeof(struct channel_map));
    map_init(eventLoop->channelMap);

#ifdef EPOLL_ENABLE
    printf("把epoll作为事件派发器, %s\n", eventLoop->thread_name);
    eventLoop->eventDispatcher = &epoll_dispatcher;
#else
    printf("把poll作为事件派发器, %s\n", eventLoop->thread_name);
    eventLoop->eventDispatcher = &poll_dispatcher;
#endif
    printf("调用事件派发器的初始化方法init().......%s\n", eventLoop->thread_name);
    eventLoop->event_dispatcher_data = eventLoop->eventDispatcher->init(eventLoop);

    //add the socketfd to event
    printf("eventLoop绑定当前线程id，在owner_thread_id成员上.......%s\n", eventLoop->thread_name);
    eventLoop->owner_thread_id = pthread_self();
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, eventLoop->socketPair) < 0) {
        LOG_ERR("socketpair set fialed");
    }
    printf("eventLoop记录线程id：%d,----,self():%d\n", eventLoop->owner_thread_id,pthread_self());

    printf("eventLoop->socketPair，生成两个内部套接字，0:%d----1:%d.....%s..\n",eventLoop->socketPair[0],eventLoop->socketPair[1],eventLoop->thread_name);
    eventLoop->is_handle_pending = 0;
    eventLoop->pending_head = NULL;
    eventLoop->pending_tail = NULL;

    printf("单独把socketPair[1]封装成channel，事件是%d，回调是handleWakeup(),data是eventLoop...%s..\n",EVENT_READ,eventLoop->thread_name);
    struct channel *channel = channel_new(eventLoop->socketPair[1], EVENT_READ, handleWakeup, NULL, eventLoop);
    event_loop_add_channel_event(eventLoop, eventLoop->socketPair[1], channel);

    return eventLoop;
}

/**
 *
 * 1.参数验证
 * 2.调用dispatcher来进行事件分发,分发完回调事件处理函数
 */
int event_loop_run(struct event_loop *eventLoop) {
    assert(eventLoop != NULL);

    struct event_dispatcher *dispatcher = eventLoop->eventDispatcher;

    if (eventLoop->owner_thread_id != pthread_self()) {
        exit(1);
    }

    printf("eventLoop 启动。%s。当前在%d\n",eventLoop->thread_name,pthread_self());
    struct timeval timeval;
    timeval.tv_sec = 1;

    while (!eventLoop->quit) {
        //block here to wait I/O event, and get active channels
        //阻塞到这里，epoll->wait监听
        dispatcher->dispatch(eventLoop, &timeval);

        //handle the pending channel
        printf("dispatch方法后，eventLoop去处理pending队列。。%s。当前是%d\n",eventLoop->thread_name,pthread_self());
        event_loop_handle_pending_channel(eventLoop);
    }

    printf("eventLoop 停止。。%s。 当前是%d\n",eventLoop->thread_name,pthread_self());
    return 0;
}


