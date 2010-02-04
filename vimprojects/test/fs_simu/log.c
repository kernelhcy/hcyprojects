#include "log.h"
#include <string.h>
//定义日志的最大长度
#define MAXLINE  4096

//定义log的类型。
typedef enum
{
	_LOG_ERROR, 	//错误信息
	_LOG_INFO, 		//一般信息
	_LOG_WARNING, 	//警告信息
	_LOG_UNKNOWN 	//未知信息
}log_t;

static void do_log(const char *fmt, log_t t, va_list ap);

void log_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	do_log(fmt, _LOG_ERROR, ap);
	va_end(ap);
	return;
}

void log_info(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	do_log(fmt, _LOG_INFO, ap);
	va_end(ap);
	return;
}

void log_warning(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	do_log(fmt, _LOG_WARNING, ap);
	va_end(ap);
	return;
}

void do_log(const char *fmt, log_t t, va_list ap)
{
	char buf[MAXLINE];
	memset(buf, '\0', sizeof(buf));

	switch (t)
	{
		case _LOG_ERROR:
			strcpy(buf, "ERROR: ");
			break;
		case _LOG_INFO:
			strcpy(buf, "INFO: ");
			break;
		case _LOG_WARNING:
			strcpy(buf, "WARNING: ");
			break;
		case _LOG_UNKNOWN:
			strcpy(buf, "UNKNOWN TYPE: ");
			break;
		default:
			fputs("ERROR IN DO_LOG!!", stderr);
			break;
	}

	vsnprintf(buf + strlen(buf), MAXLINE - strlen(buf), fmt, ap);
	strcat(buf, "\n");
	fflush(stdout);

	if (t == _LOG_ERROR)
	{
		fputs(buf, stderr);
	}
	else
	{
		fputs(buf, stdout);
	}

	fflush(NULL);

	return;
}

