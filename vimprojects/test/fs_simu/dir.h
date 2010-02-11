#ifndef __HFS_DIR_H
#define __HFS_DIR_H


/*
 * 解析目录路径，查找文件和目录。
 * 获得其inode号。 
 * 当路径有错误时，返回-1，否则返回inode号。
 */
int find(const char * path);

/*
 * 分配目录表
 * 返回分配的目录表项的索引。
 */
int diralloc();

/*
 * 释放目录项
 */
int dirfree(int id);

#endif


