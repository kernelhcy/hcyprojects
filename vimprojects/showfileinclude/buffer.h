#ifndef _BUFFER_H
#define _BUFFER_H

#include "headers.h"
#include "log.h"
/**
 * 一个buffer
 * 用来拼接字符串。
 */
typedef struct 
{
	char 	*ptr;
	size_t 	len;
	size_t 	used;
}buffer;

/*
 * 定义buffer数组
 */
typedef struct
{
	buffer 	**ptr; 			//指针数组！！！
	size_t 	len;
	size_t	used;
}buffer_array;

#define BUF_BASE_LEN 32 	//buffer的长度以及buffer数组的长度必须为其整数倍

buffer* buffer_init();
//初始化buffer的大小为n
buffer* buffer_init_n(size_t n);
//用字符串s初始化buffer的数据
buffer* buffer_init_string(const char *s);
int buffer_append(buffer *buf, const char *s, size_t s_len);
//释放buffer占用的空间
void buffer_free(buffer *buf);

buffer_array* buffer_array_init();
//初始化数组的大小为n
buffer_array* buffer_array_init_n(size_t n);
//释放数组占用的空间
void buffer_array_free(buffer_array* ba);
//追加数据到数组中
int buffer_array_append(buffer_array* ba, buffer* buf);
int buffer_array_append_string(buffer_array* ba, const char *s);
//删除数据
int buffer_array_delete(buffer_array* ba, buffer* buf);
int buffer_array_delete_string(buffer_array* ba, const char *s);

#endif
