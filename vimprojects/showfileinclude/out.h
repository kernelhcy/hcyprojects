#ifndef _OUT_H
#define _OUT_H

#include "headers.h"
#include "digraph.h"
#include "buffer.h"
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

#endif
