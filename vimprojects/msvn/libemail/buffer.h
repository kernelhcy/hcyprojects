#ifndef _MSVN_BUFFER_H
#define _MSVN_BUFFER_H

#include "headers.h"
#include "log.h"

#define BUF_BASE_LEN 32 	//buffer的长度以及buffer数组的长度必须为其整数倍

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

buffer* buffer_init();
//初始化buffer的大小为n
buffer* buffer_init_n(size_t n);
//用b初始化一个新的buffer。
buffer* buffer_init_copy(buffer *b);
//用字符串s初始化buffer的数据
buffer* buffer_init_string(const char *s);

int buffer_append(buffer *buf, const char *s, size_t s_len);

/*
 * 将buf的内容复制到a中。
 */
int buffer_copy(buffer *a, buffer *b);
/*
 * 比较两个buffer中的数据是否相同。
 * 相同返回1,否则返回0.
 */
int buffer_cmp(buffer *a, buffer *b);

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

void buffer_array_print(buffer_array *ba);
/*
 * 将字符串以"/"为分割符进行分割。
 * 将分割的结果保存在buffer数组中返回。
 */
buffer_array* splite_by_slash(const char *s);
#endif
