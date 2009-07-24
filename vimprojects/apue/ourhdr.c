#include "ourhdr.h"

void err_quit(const char *s, ...)
{

	printf("%s\n", s);
	exit(1);
}

void err_sys(const char *s, ...)
{
	printf("error!\n");
	exit(1);
}
