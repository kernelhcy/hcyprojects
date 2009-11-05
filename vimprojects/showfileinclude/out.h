#ifndef _OUT_H
#define _OUT_H

#include "headers.h"
#include "digraph.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
/**
 * 将一个有向图输出为文件。
 *
 * 通过调用dot生成图片。
 * 文本按下列格式输出：
 * 	
 * 			a------>b------>c
 * 			|       |       ^
 * 			|       |       |
 * 			V       V       |
 * 			e------>f------>g
 * 输出的文本为：
 * 			a:
 *  			b e
 * 			b:
 *  			c f
 * 			c:
 * 			e:
 *  			f
 * 			f:
 *  			g
 * 			g:
 *  			e
 *
 */

/**
 * dg是有向图
 * t 是要生成的输出文件的格式。
 * name 为输出的文件名称。
 */
int create_out(digraph *dg, type_t t, const char *name);

/**
 * 一个简易的buffer
 * 用来拼接字符串。
 */
typedef struct 
{
	char *ptr;
	size_t len;
	size_t used;
}buffer;

#define BUF_BASE_LEN 32 	//buffer的长度必须为其整数倍

buffer *buffer_init();
buffer *buffer_init_n(size_t n);
int buffer_append(buffer *buf, const char *s, size_t s_len);

#endif
