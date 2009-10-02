#include "ch1.h"

void runit(int argc, char * argv[])
{
	//p_1_1(argc, argv);	
	//p_1_2();
	//p_1_3();
	//p_1_4();
	//p_1_5();
	p_1_6(argc, argv);
	p_1_7();
	p_1_8();
}

/*
 * 程序清单1-1
 * 列出一个目录中的所有文件，目录路径从参数读取。
 */
void p_1_1(int argc, char * argv[])
{
	DIR 			*dp;
	struct dirent 	*dirp;
	
	if (argc != 2)
	{
		err_quit("usage: ls directroy_name");
	}
	
	if ((dp = opendir(argv[1])) == NULL)
	{
		err_sys("can't open %s", argv[1]);
	}
	
	while((dirp = readdir(dp)) != NULL)
	{
		printf("name : %s  \t\ttype : %d \t reclen : %d\n"
					, dirp -> d_name, dirp -> d_type, dirp -> d_reclen);
	}
	
	closedir(dp);
	
	return;
}

/*
 * 程序清单1-2
 * 将标准输入复制到标准输出。
 */
#ifndef BUFFSIZE
#define BUFFSIZE 4096
#endif 
void p_1_2()
{
	int		n;
	char	buf[BUFFSIZE];
	
	while ( (n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0 )
	{
		if (write(STDOUT_FILENO, buf, n) != n)
		{
			err_sys("write error");
		}
	}	
	
	if (n < 0)
	{
		err_sys("read error");
	}
	
	return;
}

/*
 * 程序清单1-3
 * 用标准I/O将标准输入复制到标准输出。
 */
void p_1_3()
{
	int 	c;
	
	while ((c = getc(stdin)) != EOF)
	{
		if (putc(c, stdout) == EOF)
		{
			err_sys("output error");
		}
	}
	
	if (ferror(stdin))
	{
		err_sys("input error");
	}
	
	return;
}

/*
 * 程序清单1-4
 * 打印进程ID。
 */
void p_1_4()
{
	printf("Hello UPAE! PID: %d\n", getpid());
	return;
}

/*
 * 程序清单1-5
 * 从标准输入读命令并执行。
 */
void p_1_5()
{
	char	buf[MAXLINE];	/* from apue.h */
	pid_t	pid;
	int		status;
	
	printf("%% ");			/* print prompt: % */
	while (fgets(buf, MAXLINE, stdin) != NULL)
	{
		if (buf[strlen(buf) - 1] == '\n')
		{
			buf[strlen(buf) - 1] = '\0';	/* replace newline with NULL */
		}
		
		if ((pid = fork()) < 0)
		{ 
			err_sys("fork error");
		}
		else if (pid == 0)
		{
			execlp(buf, buf, (char *)0);
			err_ret("couldn't execute: %s", buf);
			exit(127);
		}
		
		/* parent */
		if ((pid = waitpid(pid, &status, 0)) < 0)
		{	
			err_sys("waitpid error");
		}
		
		printf("%% ");
	}
	return;
}

/*
 * 程序清单1-6
 * 例示strerror和perror
 * 输出：
 *			EACCES: Permission denied
 *			./main: No such file or directory
 */
#include <errno.h>
void p_1_6(int argc, char * argv[])
{
	fprintf(stderr, "EACCES: %s\n", strerror(EACCES));
	errno = ENOENT;
	perror(argv[0]);
	return;
}

/*
 * 程序清单1-7
 * 打印用户ID和组ID
 */
void p_1_7()
{
	printf("uid = %d, gid = %d\n", getuid(), getgid());
	return;
}

/*
 * 程序清单1-8
 * 从标准输入读命令并执行
 * 引入中断信号捕捉，捕捉信号后，程序终止。Ctrl+C
 */
static void sig_int();	/* our signal-catching function */
void p_1_8()
{
	char	buf[MAXLINE];	/* from apue.h */
	pid_t	pid;
	int		status;
	
	if (signal(SIGINT, sig_int) == SIG_ERR)
	{
		err_sys("signal error");
	}
	
	printf("%% ");			/* print prompt: % */
	while (fgets(buf, MAXLINE, stdin) != NULL)
	{
		if (buf[strlen(buf) - 1] == '\n')
		{
			buf[strlen(buf) - 1] = '\0';	/* replace newline with NULL */
		}
		
		if ((pid = fork()) < 0)
		{ 
			err_sys("fork error");
		}
		else if (pid == 0)
		{
			execlp(buf, buf, (char *)0);
			err_ret("couldn't execute: %s", buf);
			exit(127);
		}
		
		/* parent */
		if ((pid = waitpid(pid, &status, 0)) < 0)
		{	
			err_sys("waitpid error");
		}
		
		printf("%% ");
	}
	return;
}

static void sig_int(int signo)
{
	printf("interrupt\n%% ");
}

