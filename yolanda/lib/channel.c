#include "channel.h"


// 一个channel代表一个连接的基础信息
/**
fd表示套接字
events表示将来交给epoll时的事件
当发生这些事件时，readCallback,writeCallback
然后，最后还有空指针类型的data结构体，每个套接字需要的数据是不一样的
    监听套接字给的是：tcpServer结构体
    连接套接字是：tcpConnection结构体

   在它们各自的回调函数中，一定会用到这个data
*/
struct channel *
channel_new(int fd, int events, event_read_callback eventReadCallback, event_write_callback eventWriteCallback,
            void *data) {
    //申请一个channel的内存
    struct channel *chan = malloc(sizeof(struct channel));
    //初始化它的几个成员，有监听套接字fd哦
    chan->fd = fd;
    chan->events = events;
    chan->eventReadCallback = eventReadCallback;
    chan->eventWriteCallback = eventWriteCallback;

    //这个是tcpConnection指针，
    chan->data = data;
    return chan;
}

int channel_write_event_is_enabled(struct channel *channel) {
    return channel->events & EVENT_WRITE;
}

int channel_write_event_enable(struct channel *channel) {
    struct event_loop *eventLoop = (struct event_loop *) channel->data;
    //加上写事件
    channel->events = channel->events | EVENT_WRITE;
    //更新到epoll中
    event_loop_update_channel_event(eventLoop, channel->fd, channel);
}

int channel_write_event_disable(struct channel *channel) {
    struct event_loop *eventLoop = (struct event_loop *) channel->data;
    channel->events = channel->events & ~EVENT_WRITE;
    event_loop_update_channel_event(eventLoop, channel->fd, channel);
}