#ifndef _SFI_PATH_H
#define _SFI_PATH_H

#include "headers.h"
#include "buffer.h"
#include "log.h"

/*
 * 定义路径数据类型。
 * 用于处理路径。
 */

typedef struct _path_t
{
	buffer *path;
	int is_dir;
	
}path_t;

/* 操作函数 */

path_t* path_init();
path_t* path_init_string(const char *s);
void path_free(path_t* path);

//将路径的基目录设置为s
int path_set_base_dir(const char *s);

//将目录调整为path。
//相对于基目录！！
int path_cd(const char *path);

/*
 * 将目录切换为下一级和上一级目录。
 * 如果成功，返回1。失败返回0.
 */
int path_child(path_t *p);
int path_parent(path_t *p);

/*
 * 返回当前路径所指向的文件的文件名。
 * 如果不是文件，则返回NULL。
 */
char* path_file(path_t *p);

#endif
