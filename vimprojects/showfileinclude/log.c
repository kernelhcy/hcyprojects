#include "log.h"
//辅助处理函数
static void log_do(log_t type, const char *fmt, va_list ap);

void log_err(const char *f, ...)
{
	va_list 	ap;
	va_start(ap, f);
	log_do(ERR, f, ap);
	va_end(ap);
}
void log_info(const char *f, ...)
{
	va_list 	ap;
	va_start(ap, f);
	log_do(INFO, f, ap);
	va_end(ap);
}
void log_warnning(const char *f, ...)
{
	va_list 	ap;
	va_start(ap, f);
	log_do(WARNNING, f, ap);
	va_end(ap);
}

static void log_do(log_t type, const char *fmt, va_list ap)
{
	char 	buf[MAXLINE];
	switch(type)
	{
		case ERR:
			strcpy(buf, "ERROR: ");
			break;
		case INFO:
			strcpy(buf, "INFO: ");
			break;
		case WARNNING:
			strcpy(buf, "WARRING: ");
			break;
		case UNKNOWN:
			strcpy(buf, "UNKNOWN LOG: ");
			break;
		default:
			fputs("ERROR IN LOG_DO!!\n", stderr);
			exit(1);
	}
	vsnprintf(buf + strlen(buf), MAXLINE - strlen(buf), fmt, ap);
	strcat(buf, "\n");
	fflush(stdout);
	if (type == ERR)
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

void assert(void *p)
{
	if ( NULL == p)
	{
		log_err("NULL pointer after malloc.");
		exit(1);
	}

}
