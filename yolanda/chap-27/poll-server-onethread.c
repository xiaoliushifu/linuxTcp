#include <lib/acceptor.h>
#include "lib/common.h"
#include "lib/event_loop.h"
#include "lib/tcp_server.h"

//这个

char rot13_char(char c) {
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        return c;
}

//连接建立之后的callback
int onConnectionCompleted(struct tcp_connection *tcpConnection) {
    printf("新连接建立完成，来个回调。。。%s\n",tcpConnection->name);
    return 0;
}

//数据读到buffer之后的callback
/**

*/
int onMessage(struct buffer *input, struct tcp_connection *tcpConnection) {
    printf("tcp连接已经来数据了 %s \n", tcpConnection->name);

    struct buffer *output = buffer_new();
    int size = buffer_readable_size(input);
    for (int i = 0; i < size; i++) {
//        buffer_append_char(output, rot13_char(buffer_read_char(input)));
        buffer_append_char(output, buffer_read_char(input));
    }
    //把output里的数据通过tcp_connection里的方法，发送给客户端
    printf("调用send方法再给客户端发送回去 %s\n", tcpConnection->name);
    tcp_connection_send_buffer(tcpConnection, output);
    return 0;
}

//数据通过buffer写完之后的callback
int onWriteCompleted(struct tcp_connection *tcpConnection) {
    printf("哪个tcp连接write完了   %s\n", tcpConnection->name);
    return 0;
}

//连接关闭之后的callback
int onConnectionClosed(struct tcp_connection *tcpConnection) {
    printf("哪个tcp连接close了 %s\n", tcpConnection->name);
    return 0;
}

int main(int c, char **v) {
    //主线程event_loop,这个得多看几回
    struct event_loop *eventLoop = event_loop_init();

    //初始化acceptor，
    //实例化服务端套接字并开启监听
    //acceptor结构体就俩成员：listen_fd就是监听的套接字成员，另一个表示监听的端口listen_port
    struct acceptor *acceptor = acceptor_init(SERV_PORT);

    //实例化tcp_server结构体，可以指定线程数目，如果线程是0，就只有一个线程，既负责acceptor，也负责I/O
    //然后给这个结构体的各个成员赋值
    //它的成员有几个重要的需要说明下：
    //最后一个参数设置线程数为0，也就是不需要子线程
    //修改最后一个参数为4，使得开启子线程，主从-reactor
    struct TCPserver *tcpServer = tcp_server_init(eventLoop, acceptor, onConnectionCompleted, onMessage,
                                                  onWriteCompleted, onConnectionClosed, 2);
    //把linten套接字封装到channel,另外注册了监听套接字linten_fd的读事件回调
    //linten_fd的读事件回调，肯定就是accept()无疑
    tcp_server_start(tcpServer);

    // main thread for acceptor
    //这里的run，不用猜，核心肯定就是epoll_wait
    event_loop_run(eventLoop);
}