#include  <sys/epoll.h>
#include "lib/common.h"

#define MAXEVENTS 128

char rot13_char(char c) {
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        return c;
}

int main(int argc, char **argv) {
    int listen_fd, socket_fd;
    int n, i;
    int efd;
    //epoll_event是一个结构体，其中的data成员也是一个结构体，（嵌套结构体）
    struct epoll_event event;
    struct epoll_event *events;

    listen_fd = tcp_nonblocking_server_listen(SERV_PORT);

    efd = epoll_create1(0);
    if (efd == -1) {
        error(1, errno, "epoll create failed");
    }

    //event结构体添加套接字
    event.data.fd = listen_fd;
    //这个套接字感兴趣的事件：可读，边缘触发
    event.events = EPOLLIN | EPOLLET;
    //添加到epoll实例上，使之可以监听
    //后续新建立连接的套接字，也是调用该方法使之可以监听
    if (epoll_ctl(efd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
        error(1, errno, "epoll_ctl add listen fd failed");
    }

    /* Buffer where events are returned */
    //这个calloc函数，如何看实现呢？
    //它应该包装了event为数组元素，返回event数组
    //还是结构体数组
    events = calloc(MAXEVENTS, sizeof(event));

    while (1) {
        //events是内核处理IO事件后设置的值，是回调参数；
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        printf("epoll_wait被唤醒了....%d个事件..\n",n);
        printf("具体哪个套接字的什么事件，请看events结构体数组..\n",n);
        for (i = 0; i < n; i++) {
                //每个监视的成员的事件，是不是异常
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))) {
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
                //监听套接字
            } else if (listen_fd == events[i].data.fd) {
                struct sockaddr_storage ss;
                socklen_t slen = sizeof(ss);
                int fd = accept(listen_fd, (struct sockaddr *) &ss, &slen);
                if (fd < 0) {
                    error(1, errno, "accept failed");
                } else {
                    make_nonblocking(fd);
                    //新的连接加入event结构体中，但是没有和events建立联系？后续怎么wait呢？
                    event.data.fd = fd;
                    //特殊之处，边缘触发！
                    event.events = EPOLLIN | EPOLLET; //edge-triggered
                    //调用ctl被epoll实例监控起来
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event) == -1) {
                        error(1, errno, "epoll_ctl add connection fd failed");
                    }
                }
                continue;
                //其他已经建立连接的套接字
            } else {
                socket_fd = events[i].data.fd;
                printf("get event on socket fd == %d \n", socket_fd);
                while (1) {
                    char buf[512];
                    //先读
                    if ((n = read(socket_fd, buf, sizeof(buf))) < 0) {
                        if (errno != EAGAIN) {
                            error(1, errno, "read error");
                            close(socket_fd);
                        }
                        break;
                    } else if (n == 0) {
                        close(socket_fd);
                        break;
                    } else {
                        //再写回去
//                        for (i = 0; i < n; ++i) {
//                            buf[i] = rot13_char(buf[i]);
//                        }
                        if (write(socket_fd, buf, n) < 0) {
                            error(1, errno, "write error");
                        }
                    }
                }
            }
        }
        printf("一次epoll_wait结束了.........\n",n);
    }

    free(events);
    close(listen_fd);
}
