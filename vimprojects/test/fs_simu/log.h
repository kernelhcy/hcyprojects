#ifndef __HFS_LOG_H
#define __HFS_LOG_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
/**
 * 打印输出日志。
 * 包括三个函数，分别输出日志信息，错误信息和警告信息。
 * 函数调用的格式和printf函数相同
 */
void log_info(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_warning(const char *fmt, ...);

#endif
