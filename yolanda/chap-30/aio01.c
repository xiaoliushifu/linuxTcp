#include "lib/common.h"
#include <aio.h>

const int BUF_SIZE = 512;

int main() {
    int err;
    int result_size;

    // 创建一个临时文件
    char tmpname[256];
    snprintf(tmpname, sizeof(tmpname), "/tmp/aio_test_%d", getpid());
    //删除tmpname表示的文件（文件全名称）
    unlink(tmpname);
    //重新打开这个文件
    int fd = open(tmpname, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        error(1, errno, "open file failed ");
    }
    //删除这个文件（其实删除不了，因为文件被打开着，待后续关闭后才能删除）
    unlink(tmpname);


    char buf[BUF_SIZE];

    //这个aiocb结构体在操作系统的aio.h中有，好好看看
    //内核如何帮你完成read和write，其实就是它有了文件描述符和偏移量，然后读到到缓冲区。
    //所以，异步就是让内核帮你完成read，那么它帮你完成read就需要这个结构体。
    struct aiocb aiocb;

    //初始化buf缓冲，写入的数据应该为0xfafa这样的,
    //memset是初始化某个变量的函数，把buf数据结构开始的BUF_SIZE个字节的内存初始化为0xfa,
    memset(buf, 0xfa, BUF_SIZE);

    //为aiocb结构体申请内存
    memset(&aiocb, 0, sizeof(struct aiocb));
    //添加fd成员
    aiocb.aio_fildes = fd;
    //添加缓冲区（字符数组）
    aiocb.aio_buf = buf;
    //缓冲区大小
    aiocb.aio_nbytes = BUF_SIZE;

    //开始写
    if (aio_write(&aiocb) == -1) {
        printf(" Error at aio_write(): %s\n", strerror(errno));
        close(fd);
        exit(1);
    }

    //因为是异步的，需要判断什么时候写完
    while (aio_error(&aiocb) == EINPROGRESS) {
        printf("writing... \n");
    }

    //判断写入的是否正确
    err = aio_error(&aiocb);
    result_size = aio_return(&aiocb);
    if (err != 0 || result_size != BUF_SIZE) {
        printf(" aio_write failed() : %s\n", strerror(err));
        close(fd);
        exit(1);
    }

    //下面准备开始读数据
    char buffer[BUF_SIZE];
    //初始化读数据结构
    struct aiocb cb;


    cb.aio_nbytes = BUF_SIZE;
    //文件指针
    cb.aio_fildes = fd;
    //偏移量
    cb.aio_offset = 0;
    //缓存
    cb.aio_buf = buffer;

    // 开始读数据，一次调用
    if (aio_read(&cb) == -1) {
        printf(" air_read failed() : %s\n", strerror(err));
        close(fd);
    }

    //因为是异步的，需要判断什么时候读完
    //轮询判断结果（问内核读完了没有）
    while (aio_error(&cb) == EINPROGRESS) {
        printf("Reading... \n");
    }

    // 判断读是否成功
    int numBytes = aio_return(&cb);
    if (numBytes != -1) {
        printf("Success.\n");
    } else {
        printf("Error.\n");
    }

    // 清理文件句柄
    close(fd);
    return 0;
}