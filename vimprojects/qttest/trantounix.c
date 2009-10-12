/*
 * 去掉程序中换行符中的'\r'
 * 将windows的换行符转换为unix的换行符
 *
 */
#include <stdlib.h>
#include <stdio.h>

void tran(const char *filename)
{
	FILE *fp = NULL;
	FILE *out = NULL;
	if ((fp = fopen(filename, "r"))== NULL)
	{
		printf("can not open file : %s.\n", filename);
		exit(1);
	}
	
	if ((out = fopen("out.cpp", "w"))== NULL)
	{
		printf("can not open outputfile.\n");
		exit(1);
	}

	char c;
	
	while ( (c = fgetc(fp)) != EOF )
	{
		if (c == '\r')
		{
			continue;
		}
		fputc(c, out);
	}

	fclose(fp);
	fclose(out);
	
	char cmd[100];
	sprintf(cmd, "mv out.cpp %s", filename);
	printf("%s\n", cmd);
	system(cmd);
}

int main(int argv, char *argc[])
{
	if (argv < 2) 
	{
		printf("need file name.\n");
		exit(1);
	}
	
	int i = 1;

	for(; i < argv; ++i)
	{
		tran(argc[i]);
	}
	return 0;
}
