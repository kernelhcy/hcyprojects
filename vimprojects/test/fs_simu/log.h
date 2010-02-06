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

/**
 * 打开日志。
 * 参数是日志文件的名称。
 * 如果参数为NULL，则向标准输入输出错误输出日志信息。
 */
void log_open(const char *file);

/**
 * 关闭日志
 */
void log_close();

#endif
