/**
 * 处理程序的日志
 * 主要用于格式化输出错误
 */
#ifndef _MSVN_LOG_H
#define _MSVN_LOG_H

#include <stdarg.h>
#include "headers.h"

#define MAXLINE 200 	//字符串缓冲区的最大长度

typedef enum _log_t
{
	ERR, INFO, WARNNING, UNKNOWN
}log_t;

void log_err(const char *f, ...);
void log_info(const char *f, ...);
void log_warnning(const char *f, ...);

/**
 * 用于断言p是否为空NULL
 */
void assert(void *p);

#endif
