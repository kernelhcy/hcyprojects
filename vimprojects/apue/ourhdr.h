/*
 * Our own header, to be included *after* all standard system headers.
 */

#ifndef __ourhdr_h
#define __ourhdr_h

#include <sys/types.h> /* required for some of our prototypes */
#include <stdio.h>     /* for convenience */
#include <stdlib.h>    /* for convenience */
#include <string.h>    /* for convenience */
#include <unistd.h>    /* for convenience */

#define MAXLINE 4096

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

void err_quit(const char *, ...);
void err_sys(const char *, ...);

#endif /* __ourhdr_h */

