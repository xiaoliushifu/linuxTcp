#include "lib/common.h"

#define MAX_LINE 1024
#define FD_INIT_SIZE 128

char rot13_char(char c) {
    //a-m时，字母置换（往后置换），
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
        //n-z时往前置换
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        //非字母不置换
        return c;
}

//数据缓冲区，这个数据结构理解好了才行
struct Buffer {
    int connect_fd;  //连接字，套接字。注意很特殊的地方在于：它是一个整型
    char buffer[MAX_LINE];  //实际缓冲，字符数组，最大1024个字节的数组嘛
    size_t writeIndex;      //缓冲写入位置
    size_t readIndex;       //缓冲读取位置
    int readable;           //是否可以读
};


//分配一个Buffer对象，初始化writeIdnex和readIndex等
struct Buffer *alloc_Buffer() {
    //申请内存，空仓库，可以往里读取数据了
    struct Buffer *buffer = malloc(sizeof(struct Buffer));
    if (!buffer)
        return NULL;
    buffer->connect_fd = 0;
    buffer->writeIndex = buffer->readIndex = buffer->readable = 0;
    return buffer;
}

//释放Buffer对象
void free_Buffer(struct Buffer *buffer) {
    free(buffer);
}

//这里从fd套接字读取数据，数据先读取到本地buf数组中，再逐个拷贝到buffer对象缓冲中
int onSocketRead(int fd, struct Buffer *buffer) {
    char buf[1024];
    int i;
    ssize_t result;
    while (1) {
        //因为是非阻塞读，所以会立即返回，需要判断buf到底读取了多少数据
        //result就是真正读取到的字节数
        result = recv(fd, buf, sizeof(buf), 0);
        printf("一次读取完成，读取字节数：%d。。。。。\n",result);
        //读取不到字节数时，才退出循环；换句话说，只要能读，就一直读出存到buffer中
        //0表示断开连接而无需再读取
        //小于0，则表示没有数据可读了，或者说数据已经读完了
        if (result <= 0)
            break;

        //按char对每个字节进行拷贝，每个字节都会先调用rot13_char来完成编码，之后拷贝到buffer对象的缓冲中，
        //其中writeIndex标志了缓冲中写的位置
        for (i = 0; i < result; ++i) {
                //这个fd对应的buffer对象还未写满，就写入buffer中，数据用rot13_char做处理
                //rot13_char大概意思就是移动位置，只替换字符而已
            if (buffer->writeIndex < sizeof(buffer->buffer))
                //在指定的数组索引位置写上数据后，索引在向后移动一位
//                buffer->buffer[buffer->writeIndex++] = rot13_char(buf[i]);
                buffer->buffer[buffer->writeIndex++] = buf[i];
            //如果读取了回车符，则认为client端发送结束，此时可以把编码后的数据回送给客户端
            if (buf[i] == '\n') {
                printf("读取到换行符，下一次这个套接字可以加入到写集合中被监听：%d。。。。。\n",fd);
                buffer->readable = 1;  //缓冲区可以读
            }
        }
    }
    //返回0，说明对方断开了连接
    if (result == 0) {
        printf("read返回0，可能客户端断开连接。%d。。。。。\n",result);
        return 1;
    //小于0退出循环
    } else if (result < 0) {
        //errno哪里来的？
        if (errno == EAGAIN)
            return 0;//有错误
        return -1;//正常读取完成
    }
    //
    return 0;
}

//从buffer对象的readIndex开始读，一直读到writeIndex的位置，这段区间是有效数据
int onSocketWrite(int fd, struct Buffer *buffer) {
    printf("%d发生可写事件。。。。。\n",fd);
    while (buffer->readIndex < buffer->writeIndex) {
        //不断重复调用send函数往里写，buffer写了多少，就向前移动多少指针，下次再send时不能重复
        //result告诉我们一次send到底写了多少
        ssize_t result = send(fd, buffer->buffer + buffer->readIndex, buffer->writeIndex - buffer->readIndex, 0);
        if (result < 0) {
            //这种表示暂时不可写吗？，返回0需要去继续处理其他文件描述符
            if (errno == EAGAIN)
                return 0;
            return -1;
        }
        printf("完成一次写动作，写入%d。。。。。\n",result);
        buffer->readIndex += result;
    }

    //readindex已经追上writeIndex，说明有效发送区间已经全部读完，将readIndex和writeIndex设置为0，复用这段缓冲
    if (buffer->readIndex == buffer->writeIndex)
        buffer->readIndex = buffer->writeIndex = 0;

    //缓冲数据已经全部读完，不需要再读
    printf("%d的readable置为0，下次将不会加入到写集合中。。。。。\n",fd);
    buffer->readable = 0;
    //返回0表示正常写入成功
    return 0;
}


int main(int argc, char **argv) {
    int listen_fd;
    int i, maxfd;

    //初始化Buffer这个结构体的数组
    struct Buffer *buffer[FD_INIT_SIZE];
    for (i = 0; i < FD_INIT_SIZE; ++i) {
        buffer[i] = alloc_Buffer();
    }
     //非阻塞监听
    listen_fd = tcp_nonblocking_server_listen(SERV_PORT);

    //三个集合并初始化0
    fd_set readset, writeset, exset;
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_ZERO(&exset);

    while (1) {
        //监听套接字很特殊
        maxfd = listen_fd;

        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_ZERO(&exset);

        // listener加入readset
        FD_SET(listen_fd, &readset);

        for (i = 0; i < FD_INIT_SIZE; ++i) {
            //遍历buffer数组，判断每个结构体对象
            if (buffer[i]->connect_fd > 0) {
                //最大的maxfd，有啥作用呢？
                if (buffer[i]->connect_fd > maxfd)
                    maxfd = buffer[i]->connect_fd;
                 //把已经连接的假如到读集合中
                 printf("加入到可读事件集合中---%d。。。。。\n",i);
                FD_SET(buffer[i]->connect_fd, &readset);
                //如果可读，则也加入到写集合中
                if (buffer[i]->readable) {
                printf("加入到可写事件集合中---%d。。。。。\n",i);
                    //这个有点奇怪，可写事件是怎么发生的呢？
                    //根据代码流程，只要加入到可写集合，就会发生可写事件，理解很困难
                    FD_SET(buffer[i]->connect_fd, &writeset);
                }
            }
        }
        //监听在select
        if (select(maxfd + 1, &readset, &writeset, &exset, NULL) < 0) {
            error(1, errno, "select error");
        }
        printf("select 。。。。。。。\n");
        //优先判断监听套接字的事件（多是客户端来连接了吧）
        if (FD_ISSET(listen_fd, &readset)) {
            printf("连接套接字可读了，说明连接过来了,sleep(5)----%d......\n",listen_fd);
            //这里这里歇息5s就是为了模拟高并发，有连接进来我也来不及去accept
            sleep(5);
            struct sockaddr_storage ss;
            socklen_t slen = sizeof(ss);
            //accept到底返回的是啥，我需要了解下，才能对后续的判断代码理解
            //返回文件描述符，但是它确实是一个整型，做php的我认为它是一个资源类型，怎么是整型呢？怪，不好理解
            //或者你可以认为它是资源对象，但是它也有一个序号属性，这个序号决定了它的排位
            int fd = accept(listen_fd, (struct sockaddr *) &ss, &slen);
            printf("accept一个fd,它是----%d......\n",fd);
            //有可能没有返回连接，非阻塞立即返回，这就是妙处
            if (fd < 0) {
                error(1, errno, "accept failed");
                //确定下，返回的是什么？不是文件描述符吗？它怎么可能和FD_INIT_SIZE比较呢？不明白
            } else if (fd > FD_INIT_SIZE) {
                error(1, 0, "too many connections");
                close(fd);
            } else {
                //已经建立的成功连接，设置为非阻塞，套接字设置为非阻塞
                //后续跟这个套接字有关的操作：读和写也都是非阻塞了
                make_nonblocking(fd);
                printf("设置非阻塞，并放到buffer中---%d。。。。。\n",fd);
                if (buffer[fd]->connect_fd == 0) {
                    buffer[fd]->connect_fd = fd;
                } else {
                    error(1, 0, "too many connections");
                }
            }
        }
        //其他事件，应该就是各个连接的读写事件了吧
        //需要依次遍历每个有效连接
        for (i = 0; i < maxfd + 1; ++i) {
            int r = 0;
            if (i == listen_fd)
                continue;
            //判断这个连接有没有可读事件
            if (FD_ISSET(i, &readset)) {
                printf("可读事件发生---%d。。。。。\n",i);
                r = onSocketRead(i, buffer[i]);
                printf("可读事件结束---r=%d。。。。。\n",r);
            }

            //有可写事件吗？r=0表示，没有读数据
            //什么场景下，发生可写事件呢？
            if (r == 0 && FD_ISSET(i, &writeset)) {
                printf("可写事件发生---%d。。。。。\n",i);
                r = onSocketWrite(i, buffer[i]);
                printf("可写事件结束---r=%d。。。。。\n",r);
            }
            if (r) {
                buffer[i]->connect_fd = 0;
                close(i);
            }
        }
        printf("结束一次select判断--%d。。。。。\n",maxfd);
    }
}
