/**
 * 处理程序的日志
 * 主要用于格式化输出错误
 */
#ifndef _FTP_LOG_H
#define _FTP_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLINE 200 	//字符串缓冲区的最大长度

//打开和关闭日志
void log_open();
void log_close();

void log_err(const char *f, ...);
void log_info(const char *f, ...);
void log_warnning(const char *f, ...);

/**
 * 用于断言p是否为空NULL
 */
void assert(void *p);

#endif
