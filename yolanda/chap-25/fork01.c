#include "lib/common.h"

#define MAX_LINE 4096

char
rot13_char(char c) {
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        return c;
}

//子进程运行的代码块
void child_run(int fd) {
    char outbuf[MAX_LINE + 1];
    size_t outbuf_used = 0;
    ssize_t result;

    while (1) {
        char ch;
        printf("子进程阻塞在recv。。。。。上-行 \n");
        result = recv(fd, &ch, 1, 0);
        printf("子进程阻塞在recv......下一行；result=%d,ch=%c \n",result,ch);
        if (result == 0) {
            break;//子进程完成退出
        } else if (result == -1) {
            perror("read");
            break;
        }

        /* We do this test to keep the user from overflowing the buffer. */
        if (outbuf_used < sizeof(outbuf)) {
//            outbuf[outbuf_used++] = rot13_char(ch);
        printf("子进程读出ch=%c 加到outbuf中\n",ch);
            outbuf[outbuf_used++] = ch;
        }
        //这样就能看出多进程的输出不一样了
        sleep(3);

        if (ch == '\n') {
            printf("子进程send ...%s\n",outbuf);
            send(fd, outbuf, outbuf_used, 0);
            outbuf_used = 0;
            continue;
        }
    }
}

//信号监听函数，处理子进程的退出
void sigchld_handler(int sig) {
    printf("信号处理函数，在主进程.....sig=%d.. \n",sig);
    while (waitpid(-1, 0, WNOHANG) > 0);
    return;
}

int main(int c, char **v) {
    int listener_fd = tcp_server_listen(SERV_PORT);
    signal(SIGCHLD, sigchld_handler);
    while (1) {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        printf("主进程阻塞在accept.....上一行..\n");
        int fd = accept(listener_fd, (struct sockaddr *) &ss, &slen);
        printf("主进程阻塞在accept....下一行...%d \n", fd);
        if (fd < 0) {
            error(1, errno, "accept failed");
            exit(1);
        }

        if (fork() == 0) {
            //这是子进程部分，不需要的资源可以减少进程计数比如【监听套接字】
            close(listener_fd);
            printf("开启一个子进程....... \n");
            child_run(fd);
            exit(0);
        } else {
            printf("这是主进程的fork_else....... \n");
            close(fd);
        }
        printf("主进程结束一次while循环....... \n");
    }
    return 0;
}