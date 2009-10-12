#include "apue.h"
#include <errno.h>		/* for definition of errno*/
#include <stdarg.h>		/* ISO C variable aruments */

static void err_doit(int, int, const char *, va_list);

/*
 * Nonfatal error related to a system call.
 * Print a message and return.
 */
void err_ret(const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
}

/*
 * Fatal error related to a system call.
 * Print a message and terminate.
 */
void err_sys(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	exit(1);
}

/*
 * Fatal error urelated to a system call.
 * Error code passed as explict parameter.
 * Print a message and terminate.
 */
void err_exit(int error, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	exit(1);
}

/*
 * Fatal error related to a system call.
 * Print a message, dump core, and terminate.
 */
void err_dump(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	abort();	/* dump core and terminate. */
	exit(1);	/* shouldn't get here */
}

/*
 * Nonfatal error unrelated to a system call.
 * Print a message and return.
 */
void err_msg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
}

/*
 * Fatal error unrelated to a system call.
 * Print a message and terminate.
 */
void err_quit(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
	exit(1);
}

/*
 * Print a message and return to caller.
 * Caller specifies "errnoflag".
 */
static void err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
	char buf[MAXLINE];
	vsnprintf(buf, MAXLINE, fmt, ap);
	
	if(errnoflag)
	{	
		snprintf(buf + strlen(buf), MAXLINE - strlen(buf), ": %s", strerror(error));
	}
	
	strcat(buf, "\n");
	fflush(stdout);	/* in cast stdout and stderr are the same */
	fputs(buf, stderr);
	fflush(NULL);	/* flushes all stdio output stream */
}

#include <syslog.h>		/* log */

static void log_doit(int, int, const char *, va_list ap);

/*
 * Caller must define and set this : nonzero if
 * interactive , zero if deamon
 */
extern int log_to_stderr;

/*
 * Initialize syslog(), if running as deamon
 */
void log_open(const char *ident, int option, int facility)
{
	if (log_to_stderr == 0)
	{
		openlog(ident, option, facility);
	}
}

/*
 * Nonfatal error related to a system call.
 * Print a message with the system's errno value and return
 */
void log_ret(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	log_doit(1, LOG_ERR, fmt, ap);
	
	va_end(ap);	
}

/*
 * Fatal error related to a system call.
 * Print a message and terminate.
 */
void log_sys(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	log_doit(1, LOG_ERR, fmt, ap);
	
	va_end(ap);
	exit(2);	
}


/*
 * Nonfatal error unrelated to a system call.
 * Print a message and return
 */
void log_msg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	log_doit(0, LOG_ERR, fmt, ap);
	
	va_end(ap);	
}

/*
 * Fatal error unrelated to a system call.
 * Print a message and terminate.
 */
void log_quit(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	log_doit(0, LOG_ERR, fmt, ap);
	
	va_end(ap);
	exit(2);	
}
 
/*
 * Print a message and return to caller.
 * Caller specifies "errnoflag" and "priority"
 */
static void log_doit(int errnoflag, int priority, const char *fmt, va_list ap)
{
	int errno_save;
	char buf[MAXLINE];
	
	errno_save = errno;
	vsnprintf(buf, MAXLINE, fmt, ap);
	if (errnoflag)
	{
		snprintf(buf + strlen(buf), MAXLINE - strlen(buf), ": %s", strerror(errno_save));
	}
	strcat(buf, "\n");
	if (log_to_stderr)
	{
		fflush(stdout);
		fputs(buf, stderr);
		fflush(stderr);
	}
	else
	{
		syslog(priority, buf);
	}
	
}


