#include <assert.h>
#include "common.h"
#include "tcp_server.h"
#include "thread_pool.h"
#include "utils.h"
#include "tcp_connection.h"

int tcp_server(int port) {
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listenfd, LISTENQ);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    signal(SIGPIPE, SIG_IGN);

    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((connfd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len)) < 0) {
        error(1, errno, "bind failed ");
    }

    return connfd;
}


int tcp_server_listen(int port) {
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listenfd, LISTENQ);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    signal(SIGPIPE, SIG_IGN);

    return listenfd;
}


int tcp_nonblocking_server_listen(int port) {
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    make_nonblocking(listenfd);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    int rt1 = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listenfd, LISTENQ);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    signal(SIGPIPE, SIG_IGN);

    return listenfd;
}

void make_nonblocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}


//TCPserver结构体的初始化
struct TCPserver *
tcp_server_init(struct event_loop *eventLoop, struct acceptor *acceptor,
                connection_completed_call_back connectionCompletedCallBack,
                message_call_back messageCallBack,
                write_completed_call_back writeCompletedCallBack,
                connection_closed_call_back connectionClosedCallBack,
                int threadNum) {
                //实例化这个结构体
    struct TCPserver *tcpServer = malloc(sizeof(struct TCPserver));
    //给它的成员赋值
    tcpServer->eventLoop = eventLoop;
    tcpServer->acceptor = acceptor;
    //四个回调函数的指针
    tcpServer->connectionCompletedCallBack = connectionCompletedCallBack;
    tcpServer->messageCallBack = messageCallBack;
    tcpServer->writeCompletedCallBack = writeCompletedCallBack;
    tcpServer->connectionClosedCallBack = connectionClosedCallBack;
    //线程数
    tcpServer->threadNum = threadNum;
    //线程池？？？
    printf("线程池结构体的初始化,数量:%d... \n",threadNum);
    tcpServer->threadPool = thread_pool_new(eventLoop, threadNum);
    tcpServer->data = NULL;
    return tcpServer;
}

/**
这个函数虽然定义在这里，但是它的函数名被用作指针实参
在tcp_server_start函数中使用，handle_connection_established指针
这个函数的功能：
    accept一个客户端连接
    把这个连接初始化tcp_connection（尝试交给子线程eventLoop,如果开启线程池的话）
*/
int handle_connection_established(void *data) {

    printf("listen_fd的读事件处理函数 accept新连接。。。。。\n");
    struct TCPserver *tcpServer = (struct TCPserver *) data;
    struct acceptor *acceptor = tcpServer->acceptor;
    int listenfd = acceptor->listen_fd;
//    printf("我是监听套接字fd:%d的读事件处理函数，执行handle_connection_established() \n",acceptor->listen_fd);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connected_fd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len);
    //设置非阻塞
    make_nonblocking(connected_fd);

    // choose event loop from the thread pool
    printf("新连接fd:%d到来，调用尝试从线程池中捞一个eventLoop.......\n", connected_fd);
    struct event_loop *eventLoop = thread_pool_get_loop(tcpServer->threadPool);

    // create a new tcp connection
    printf("新连接fd:%d要初始化tcpConnection，绑定eventLoop\n",connected_fd);
    struct tcp_connection *tcpConnection = tcp_connection_new(connected_fd, eventLoop,
                                                              tcpServer->connectionCompletedCallBack,
                                                              tcpServer->connectionClosedCallBack,
                                                              tcpServer->messageCallBack,
                                                              tcpServer->writeCompletedCallBack);
    //for callback use
    if (tcpServer->data != NULL) {
        printf("tcpServer的data成员，交给tcpConnection的data成员管理 \n");
        tcpConnection->data = tcpServer->data;
    }
    printf("listen_fd的读事件处理函数结束\n");
    return 0;
}


/**
    其实服务端套接字监听已经在acceptor_init函数里完成了
    那么这里的tcp_server_start到底完成什么呢？

        1。启动线程池(如果配置为0，则无需启动线程池，就是普通单线程）
        2。把监听套接字包装成channel

*/
void tcp_server_start(struct TCPserver *tcpServer) {
    printf("主线程tcp_server_start()。。。。。。%s \n",pthread_self());
    struct acceptor *acceptor = tcpServer->acceptor;
    struct event_loop *eventLoop = tcpServer->eventLoop;

    //开启多个线程，线程数为0时，不开启线程池
    //这就是框架的好处，通过参数的配置，灵活的使用线程池或者不用线程池

    printf("tcpServer中开始启动线程池。。。。。。 \n\n\n");
    thread_pool_start(tcpServer->threadPool);
    printf("\n\n\n线程池已经启动。。。。。。 \n");

    //acceptor主线程， 同时把tcpServer作为参数传给channel对象
    //实例化channel并初始化channel_new的成员
    //channel也有指向监听套接字的指针成员
    //这其实就是封装过程，我们看前三个参数：哪个fd,感兴趣的事件，发生事件时的响应回调
    printf("listen_fd:%d封装为channel,感兴趣事件event:%d，data是tcpServer %s \n",acceptor->listen_fd,EVENT_READ,eventLoop->thread_name);
    struct channel *channel = channel_new(acceptor->listen_fd, EVENT_READ, handle_connection_established, NULL,
                                          tcpServer);
    //把channel和eventLoop建立关系，啥关系呢？？
    //channel注册到eventLoop，建立关系
    printf("监听套接字listen_fd:%d，加入pending队列。。 \n",acceptor->listen_fd);
    event_loop_add_channel_event(eventLoop, channel->fd, channel);
    return;
}

//设置callback数据
void tcp_server_set_data(struct TCPserver *tcpServer, void *data) {
    if (data != NULL) {
        tcpServer->data = data;
    }
}
