//
// Created by shengym on 2019-07-07.
//

#include "lib/common.h"

static int count;

//发生信号后的处理函数，参数就是具体的信号
static void recvfrom_int(int signo) {
    printf("\nreceived %d datagrams\n", count);
    exit(0);
}


int main(int argc, char **argv) {
    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //固定在43211上
    server_addr.sin_port = htons(SERV_PORT);

    bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    socklen_t client_len;
    char message[MAXLINE];
    count = 0;

    //监听某信号，发生该信号时交给recvfrom_int处理
    signal(SIGINT, recvfrom_int);

    struct sockaddr_in client_addr;
    client_len = sizeof(client_addr);
    for (;;) {
        int n = recvfrom(socket_fd, message, MAXLINE, 0, (struct sockaddr *) &client_addr, &client_len);
        fputs("recvfrom。。。。。。\n", stdout);
        message[n] = 0;
        printf("received %d bytes: %s\n", n, message);

        //初始化发生数据结构
        char send_line[MAXLINE];
        //格式化发送的数据
        sprintf(send_line, "Hi, %s", message);
        //调用sendto发送出去
        sendto(socket_fd, send_line, strlen(send_line), 0, (struct sockaddr *) &client_addr, client_len);
        fputs("sendto。。。。。。\n", stdout);
        count++;
    }

}


