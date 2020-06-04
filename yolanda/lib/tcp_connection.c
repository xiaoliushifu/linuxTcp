#include "tcp_connection.h"
#include "utils.h"


int handle_connection_closed(struct tcp_connection *tcpConnection) {

    printf("执行handle_connection_closed()函数，... \n");
    struct event_loop *eventLoop = tcpConnection->eventLoop;
    struct channel *channel = tcpConnection->channel;
    printf("执行event_loop_remove_channel_event()函数，... \n");
    event_loop_remove_channel_event(eventLoop, channel->fd, channel);
    if (tcpConnection->connectionClosedCallBack != NULL) {
        printf("tcpConnection断开连接的回调走一下，... \n");
        tcpConnection->connectionClosedCallBack(tcpConnection);
    }
}
/**

    有连接之后，处理这个事件
*/
int handle_read(void *data) {
    //data就是tcp_connection数据结构
    struct tcp_connection *tcpConnection = (struct tcp_connection *) data;
    struct buffer *input_buffer = tcpConnection->input_buffer;
    struct channel *channel = tcpConnection->channel;

    if (buffer_socket_read(input_buffer, channel->fd) > 0) {
        //应用程序真正读取Buffer里的数据
        if (tcpConnection->messageCallBack != NULL) {
            tcpConnection->messageCallBack(input_buffer, tcpConnection);
        }
    } else {
        printf("tcpConnection-fd:%d,读取返回不大于0,准备断开连接.. \n",channel->fd);
        handle_connection_closed(tcpConnection);
    }
}

//发送缓冲区可以往外写
//把channel对应的output_buffer不断往外发送
int handle_write(void *data) {
    struct tcp_connection *tcpConnection = (struct tcp_connection *) data;
    struct event_loop *eventLoop = tcpConnection->eventLoop;
    assertInSameThread(eventLoop);

    struct buffer *output_buffer = tcpConnection->output_buffer;
    struct channel *channel = tcpConnection->channel;

    ssize_t nwrited = write(channel->fd, output_buffer->data + output_buffer->readIndex,
                            buffer_readable_size(output_buffer));
    if (nwrited > 0) {
        //已读nwrited字节
        output_buffer->readIndex += nwrited;
        //如果数据完全发送出去，就不需要继续了
        if (buffer_readable_size(output_buffer) == 0) {
            channel_write_event_disable(channel);
        }
        //回调writeCompletedCallBack
        if (tcpConnection->writeCompletedCallBack != NULL) {
            tcpConnection->writeCompletedCallBack(tcpConnection);
        }
    } else {
        yolanda_msgx("handle_write for tcp connection %s", tcpConnection->name);
    }

}

/**

    新连接来了哦
*/
struct tcp_connection *
tcp_connection_new(int connected_fd, struct event_loop *eventLoop,
                   connection_completed_call_back connectionCompletedCallBack,
                   connection_closed_call_back connectionClosedCallBack,
                   message_call_back messageCallBack, write_completed_call_back writeCompletedCallBack) {

                   printf("新连接来了，新构建tcpconnection----%d。。。\n", connected_fd);

    struct tcp_connection *tcpConnection = malloc(sizeof(struct tcp_connection));
    //每新来一个连接，都把指针填满
    tcpConnection->writeCompletedCallBack = writeCompletedCallBack;
    tcpConnection->messageCallBack = messageCallBack;
    tcpConnection->connectionCompletedCallBack = connectionCompletedCallBack;
    tcpConnection->connectionClosedCallBack = connectionClosedCallBack;

    //重点，它也把eventLoop这个结构体存起来，eventLoop真是全局变量吧
    tcpConnection->eventLoop = eventLoop;
    tcpConnection->input_buffer = buffer_new();
    tcpConnection->output_buffer = buffer_new();


    char *buf = malloc(20);
    sprintf(buf, "tcpConnection-fd:%d", connected_fd);
    tcpConnection->name = buf;

    // add event read for the new connection
    //新来都连接，把连接套接字connected_fd封装为channel（它关心的事件EVENT_READ，读事件回调，写事件回调，tcp
    printf("连接fd:%d封装为channel,感兴趣事件event:%d，data是tcpConnection \n",connected_fd,EVENT_READ);
    struct channel *channel1 = channel_new(connected_fd, EVENT_READ, handle_read, handle_write, tcpConnection);

    //互相连接，
        //channel1的data是tcpConnection;     而tcpConnection的channel就是channel1
    tcpConnection->channel = channel1;

    //connectionCompletedCallBack callback
    if (tcpConnection->connectionCompletedCallBack != NULL) {
        //连接建立完成之后，调用一个回调，传递tcpConnection
        tcpConnection->connectionCompletedCallBack(tcpConnection);
    }
    printf("新连接fd:%d触发add_channel事件... \n",connected_fd);
    event_loop_add_channel_event(tcpConnection->eventLoop, connected_fd, tcpConnection->channel);
    return tcpConnection;
}

/**
    由send_buffer而来
    向客户端连接发送数据的最终函数
*/
int tcp_connection_send_data(struct tcp_connection *tcpConnection, void *data, int size) {

    printf("tcpConnect的tcp_connection_send_data方法 %s\n", tcpConnection->name);
    size_t nwrited = 0;
    size_t nleft = size;
    int fault = 0;

    struct channel *channel = tcpConnection->channel;
    struct buffer *output_buffer = tcpConnection->output_buffer;

    //先往套接字尝试发送数据
    printf("channel角度看看能否写..... %s\n", tcpConnection->name);
    if (!channel_write_event_is_enabled(channel) && buffer_readable_size(output_buffer) == 0) {
        //调用write函数
        printf("channel可写...最终调用write方法写给客户端. %s\n", tcpConnection->name);
        nwrited = write(channel->fd, data, size);
        if (nwrited >= 0) {
            nleft = nleft - nwrited;
        } else {
            nwrited = 0;
            if (errno != EWOULDBLOCK) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    fault = 1;
                }
            }
        }
    }

    if (!fault && nleft > 0) {
        //拷贝到Buffer中，Buffer的数据由框架接管
        printf("一次性没有写完，放到它的output_buffer中，下回在写... %s\n", tcpConnection->name);
        buffer_append(output_buffer, data + nwrited, nleft);
        //没写完，也许这个channel的写事件根本没有监听
        if (!channel_write_event_is_enabled(channel)) {
            printf("打开channel的写事件（注册到epoll中）... %s\n", tcpConnection->name);
            channel_write_event_enable(channel);
        }
    }

    return nwrited;
}

/**
    发送消息
*/
int tcp_connection_send_buffer(struct tcp_connection *tcpConnection, struct buffer *buffer) {
    printf("tcpConnect的tcp_connection_send_buffer方法 %s\n", tcpConnection->name);
    int size = buffer_readable_size(buffer);
        //向这个连接发送数据
    int result = tcp_connection_send_data(tcpConnection, buffer->data + buffer->readIndex, size);
    buffer->readIndex += size;
    return result;
}

void tcp_connection_shutdown(struct tcp_connection *tcpConnection) {
    if (shutdown(tcpConnection->channel->fd, SHUT_WR) < 0) {
        yolanda_msgx("tcp_connection_shutdown failed, socket == %d", tcpConnection->channel->fd);
    }
}