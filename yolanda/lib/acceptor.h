#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "common.h"

//这个结构体很简单，只有一个端口，一个套接字
struct acceptor{
    int listen_port;
    int listen_fd;
} ;
//声明一个函数，函数名称是acceptor_init，需要一个port参数
//另外，这个函数的返回值是个指针，指针数据类型是结构体acceptor
//也就是说acceptor_init函数返回一个指向acceptor结构体的指针
struct acceptor *acceptor_init(int port);


#endif