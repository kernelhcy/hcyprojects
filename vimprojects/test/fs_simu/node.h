#ifndef __HFS_NODE_H
#define __HFS_NODE_H
/**
 * 定义i节点，磁盘i节点，以及物理块的实现和操作。
 * by hcy
 */
#include "fs_base.h"


/*
 * 获得和释放一个i节点。
 * 获得i节点将返回一个空闲的i节点的下标。分配失败将返回-1
 * 释放i节点，将其标记为空闲
 */
int ialloc();
void ifree(int);

/*
 * 获取和释放物理块
 */
int balloc();
void bfree(int);

#endif
