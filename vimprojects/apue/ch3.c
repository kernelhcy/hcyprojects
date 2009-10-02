#include "ch3.h"

void runit(int argc, char * argv[])
{
	p_3_1();	
	p_3_2();
	//p_3_3();
	//p_3_4();
	//p_3_5();
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
void p_3_4()
{

}
void p_3_5()
{
	
}
