#include "ch3.h"
 
void runit(int argc, char * argv[])
{
	//p_3_1();	
	//p_3_2();
	//p_3_4(argc, argv);
	//p_3_3();
	p_3_5();
}

/*
 * 程序3-1
 * 测试能否对标准输入设置偏移量
 */
void p_3_1()
{
	if (lseek(STDIN_FILENO, 0, SEEK_CUR) == -1)
	{
		printf("cannot seek\n");
	}
	else
	{
		printf("seek OK\n");
	}
	
	return;
}

/*
 * 程序3-2
 * 创建一个具有空洞的文件
 */
char	buf1[] = "abcdefghij";
char	buf2[] = "ABCDEFGHIJ";
void p_3_2()
{
	int 	fd;
	if ((fd = creat("file.hole", FILE_MODE)) < 0)
	{
		err_sys("creat error");
	}
	
	if (write(fd, buf1, 10) != 10)
	{
		err_sys("buf1 write error");
	}
	
	/* offset now = 10 */
	
	if ((lseek(fd, 16384, SEEK_SET)) == -1)
	{
		err_sys("lseek error");
	}
	
	/* offset now = 16384 */
	
	if (write(fd, buf2, 10) != 10)
	{
		err_sys("buf2 write error");
	} 
	
	/* offset now = 16394 */
	
	system("ls -l file.hole");
	/* od命令观察文件的实际内容，-c参数表示以字符的方式打印文件内容 */
	system("od -c file.hole");
	return;
}


/*
 * 程序3-3
 * 将标准输入复制到标准输出
 */
#define BUFFSIZE	4096
void p_3_3()
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
 * 程序3-4
 * 对于指定的描述符打印文件标志
 */
void p_3_4(int argc, char *argv[])
{
	int 	val;
	
	if (argc != 2)
	{
		err_quit("usage: main <descriptor#>");
	}
	
	if ((val = fcntl(atoi(argv[1]), F_GETFL, 0)) < 0)
	{	
		err_sys("fcntl error for fd : %d", atoi(argv[1]));
	}
	
	switch(val & O_ACCMODE)
	{
		case O_RDONLY:
			printf("read only ");
			break;
		case O_WRONLY:
			printf("write only ");
			break;
		case O_RDWR:
			printf("read write ");
			break;
		default:
			err_dump("unknown access mode");
	}
	
	if (val & O_APPEND)
	{
		printf(", append");
	}
	
	if (val & O_NONBLOCK)
	{
		printf(", nonblocking");
	}
	
	#if defined(O_SYNC)
	if (val & O_SYNC)
	{
		printf(", synchronous writes");
	}
	#endif
	
	#if !defined(_POSIX_C_SOURCE) && defined(O_FSYNC)
	if (val & O_FSYNC)
	{
		printf(", synchronous writes");
	}
	#endif
	
	putchar('\n');
	
	return;
	
}

/*
 * 程序3-5
 * 对一个文件描述符打开一个或多个文件状态标志
 */
void set_fl(int fd, int flags)/* flags are file status flags to trun on */
{
	int 	val;
	
	if ((val = fcntl(fd, F_GETFL, 0)) < 0)
	{
		err_sys("fcntl F_GETFL error");
	}
	
	val |= flags;	/* turn on flags */
	
	if (fcntl(fd, F_SETFL, val) < 0)
	{
		err_sys("fcntl F_SETFL error");
	}
	
	return;
}
void p_3_5()
{
	int fd = 100;
	int flags;
	
	set_fl(fd, flags);
}
