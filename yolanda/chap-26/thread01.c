#include "lib/common.h"

extern void loop_echo(int);

void thread_run(void *arg) {
    //分离线程（使得后续线程退出时自己回收资源）
    pthread_detach(pthread_self());
    int fd = (int) arg;
    //进入业务逻辑（一般是一个死循环，需要监听客户端断开连接，然后break）
    loop_echo(fd);
    printf("loop之下工作线程结束 %d \n",pthread_self());
}

int main(int c, char **v) {
    int listener_fd = tcp_server_listen(SERV_PORT);
    pthread_t tid;

    while (1) {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        printf("主线程会阻塞到这里 上 \n");
        int fd = accept(listener_fd, (struct sockaddr *) &ss, &slen);
        printf("主线程会阻塞到这里 下 \n");
        if (fd < 0) {
            error(1, errno, "accept failed");
        } else {
            pthread_create(&tid, NULL, &thread_run, (void *) fd);
        }
    }

    return 0;
}