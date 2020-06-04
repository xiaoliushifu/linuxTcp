#ifndef CHANNEL_MAP_H
#define CHANNEL_MAP_H


#include "channel.h"

/**
 * channel映射表, key为对应的socket描述字
 */
struct channel_map {

    //这个成员可以看成数组，以具体的套接字为下标（套接字是整型哦），
    //而套接字是被channel封装的，所以channel_map就是通过套接字找到封装它的channel结构体对象
    //什么时候建立channel_map和channel的关系呢？？？
    void **entries;

    //entries的数量
    /* The number of entries available in entries */
    int nentries;
};


int map_make_space(struct channel_map *map, int slot, int msize);

void map_init(struct channel_map *map);

void map_clear(struct channel_map *map);

#endif