#include "lib/common.h"

#define INIT_SIZE 128

int main(int argc, char **argv) {
    int listen_fd, connected_fd;
    int ready_number;
    ssize_t n;
    char buf[MAXLINE];
    char *sufix = "--fromServer";
    struct sockaddr_in client_addr;

    listen_fd = tcp_server_listen(SERV_PORT);

    //初始化pollfd数组，这个数组的第一个元素是listen_fd，其余的用来记录将要连接的connect_fd
    struct pollfd event_set[INIT_SIZE];
    event_set[0].fd = listen_fd;
    //需要监听的事件，【连接到来，数据到来】
    event_set[0].events = POLLRDNORM;

    // 用-1表示这个数组位置还没有被占用
    int i;
    for (i = 1; i < INIT_SIZE; i++) {
        event_set[i].fd = -1;
    }

    for (;;) {
        //开始监听：文件描述符结构体数组，数组大小，-1表示阻塞（监听的描述符没有io事件发生，就阻塞）
        //返回 大于0，等于0，小于0
        if ((ready_number = poll(event_set, INIT_SIZE, -1)) < 0) {
            error(1, errno, "poll failed ");
        }
        printf("poll.......%d \n", ready_number);

        //优先判断第一个文件描述符的事件，是不是POLLRDNORM，这里其实只能是连接到来
        if (event_set[0].revents & POLLRDNORM) {
            socklen_t client_len = sizeof(client_addr);
            //调用accept产生连接描述符，并把它假如数组，以后
            //就可以监听这个已经连接套接字的事件（读
            connected_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &client_len);

            //找到一个可以记录该连接套接字的位置
            for (i = 1; i < INIT_SIZE; i++) {
                if (event_set[i].fd < 0) {
                    event_set[i].fd = connected_fd;
                    //对已经建立连接对套接字，感兴趣的事件是【POLLRDNORM：读数据】
                    event_set[i].events = POLLRDNORM;
                    break;
                }
            }
            //连接满了
            if (i == INIT_SIZE) {
                error(1, errno, "can not hold so many clients");
            }
             //处理一个事件，就减一；
            if (--ready_number <= 0)
                continue;
        }
        //不是第一个，那就是已经建立连接的套接字的事件了，比如【读数据，FIN等】
        //需要自己依次遍历，到底哪个套接字，并且是具体什么事件
        for (i = 1; i < INIT_SIZE; i++) {
            int socket_fd;
            //从数组中过滤出已经监听的套接字
            if ((socket_fd = event_set[i].fd) < 0)
                continue;
                //再判断是不是这个套接字的事件
            if (event_set[i].revents & (POLLRDNORM | POLLERR)) {
                //读取数据，并写回
                if ((n = read(socket_fd, buf, MAXLINE)) > 0) {
                    //拼接上固定的字符串
//                    strcat(buf, sufix);
//                    printf("buf is %s\n",buf);
//                    n = strlen(buf);
                    if (write(socket_fd, buf, n) < 0) {
                        error(1, errno, "write error");
                    }
                    //套接字关闭连接
                } else if (n == 0 || errno == ECONNRESET) {
                    close(socket_fd);
                    //从数组中剔除
                    event_set[i].fd = -1;
                } else {
                    error(1, errno, "read error");
                }
                //处理一个事件，就减一；
                if (--ready_number <= 0)
                    break;
            }
        }
    }
}
