#include "lib/common.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        error(1, 0, "usage: batchwrite <IPaddress>");
    }

    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    socklen_t server_len = sizeof(server_addr);
    //建立连接
    int connect_rt = connect(socket_fd, (struct sockaddr *) &server_addr, server_len);
    if (connect_rt < 0) {
        error(1, errno, "connect failed ");
    }

    char buf[128];
    struct iovec iov[2];

    char *send_one = "hello,";
    //结构体数组，每个数组元素都是结构体
    //数组第一个元素的数据一直是hello;
    iov[0].iov_base = send_one;
    iov[0].iov_len = strlen(send_one);

    //第二个元素则绑定一个buf的缓存，从标准输入读取
    iov[1].iov_base = buf;
    while (fgets(buf, sizeof(buf), stdin) != NULL) {

        //读取到buf之后，就放到数组第二个元素中
        iov[1].iov_len = strlen(buf);
        int n = htonl(iov[1].iov_len);
        //把整个数组信息发送出去，我们看到每次变化的都是数组元素2的值
        //它是从标准输入读取而来，而数组元素0则是固定不变的hello
        //所以一直发送的是hello ,xxxxx
        //这个算法的意思是说，我们不要一有数据就发送，而是在用户态就要有缓存机制
        //把数据堆砌到一定大小时在交给内核发送，尽量避免大批量的小数据发送；
        //所谓小数据，就是小于TCP的MSS的tcp分组，可能内核会启动negal算法
        if (writev(socket_fd, iov, 2) < 0)
            error(1, errno, "writev failure");
    }
    exit(0);
}

