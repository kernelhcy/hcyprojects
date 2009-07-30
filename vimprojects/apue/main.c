#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include "ourhdr.h"
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{

	printf("Hello UPAE! PID: %d\n", getpid());

	DIR *dp;
	//struct dirent *dirp;

	//if(argc != 2)
	//{
//		err_quit("a single argument (the directory name) is reqired\n");
//	}
//
//	if((dp = opendir(argv[1])) == NULL)
//	{
//		err_sys("can't open %s", argv[1]);
//	}
//
//	while( (dirp = readdir(dp)) != NULL)
//	{
//		printf("%s\n", dirp->d_name);
//	}
//
//	closedir(dp);
	struct stat buf;
	lstat("main.o", &buf);
	if(S_ISREG(buf.st_mode))
	{
		printf("regular\n");
	}
	

	return 0;
}
