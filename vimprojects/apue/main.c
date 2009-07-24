#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include "ourhdr.h"

int main(int argc, char *argv[])
{

	printf("Hello UPAE! PID: %d\n", getpid());

	sleep(999999);
	DIR *dp;
	struct dirent *dirp;

	if(argc != 2)
	{
		err_quit("a single argument (the directory name) is reqired\n");
	}

	if((dp = opendir(argv[1])) == NULL)
	{
		err_sys("can't open %s", argv[1]);
	}

	while( (dirp = readdir(dp)) != NULL)
	{
		printf("%s\n", dirp->d_name);
	}

	closedir(dp);
	return 0;
}
