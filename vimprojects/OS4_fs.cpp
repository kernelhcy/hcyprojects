#include "headers.h"
#include "fs.h"

int init();

/*
 * 程序入口
 */
void run(bool show_details)
{
	system("clear");//清屏

	printf("HCY's Fils System.\n");
	printf("Version 0.001. \n");
	
	init();

	printf("Input \"help or h\" for help infromation.\n");
	
	char cmd[10];

	while(true)
	{
		printf("cmd:");
		scanf("%s",cmd);
		
		if(strcmp("quit",cmd) == 0|| strcmp("q", cmd) == 0)
		{
			break;
		}
		else if(strcmp("help", cmd) == 0 || strcmp("h", cmd) == 0)
		{
			show_help_info();
		}
		else if(strcmp("ls", cmd) == 0)
		{
			printf("显示当前目录内容。\n");
		}
		else if(strcmp("mkdir", cmd) == 0)
		{
			printf("创建目录。\n");
		}
		else if(strcmp("rmdir", cmd) == 0)
		{
			printf("删除目录。\n");
		}
		else if(strcmp("chdir", cmd) == 0 || strcmp("cd", cmd) == 0)
		{
			printf("更换当前工作目录。\n");
		}
		else if(strcmp("create", cmd) ==0 )
		{
			printf("创建文件。\n");
		}
		else if(strcmp("delete", cmd) == 0)
		{
			printf("删除文件。\n");
		}
		else if(strcmp("open", cmd) == 0)
		{
			printf("打开文件。\n");
		}
		else if(strcmp("close", cmd) == 0)
		{
			printf("关闭文件。\n");
		}
		else if(strcmp("write", cmd) == 0)
		{
			printf("向文件中写入数据。\n");
		}
		else if(strcmp("read", cmd) == 0)
		{
			printf("读文件中的数据。\n");
		}
		else if(strcmp("pwd", cmd) == 0)
		{
			printf("显示当前工作目录。\n");
		}
		else if(strcmp("login", cmd) == 0)
		{
			printf("登录。\n");
		}
		else if(strcmp("logout", cmd) == 0)
		{
			printf("登出。\n");
		}
		else if(strcmp("cat", cmd) == 0)
		{
			printf("显示文件内容。\n");
		}
		else
		{
			printf("No such command!\nType \"help or h\" for help information.\n");
			continue;
		}

	}


	if(show_details)
	{
		printf( "Show details.\n");
	}
}

/*
 * 初始化文件系统
 */
int init()
{
	std::cout << "初始化文件系统...\n";
	return 0;
}

/************************************/

char * ls(char *path)
{
	std::cout << "ls\n";
	return 0;
}
int mkdir(char *path)
{
	std::cout << "mkdir\n";
	return 0;
}
int rmdir(char *path)
{
	std::cout << "rmdir\n";
	return 0;
}
int chdir(char *path)
{
	std::cout << "chdir\n";
	return 0;
}
int create_f(char *name, int mode)
{
	std::cout << "create\n";
	return 0;
}
int delete_f(char *name)
{
	std::cout << "delete\n";
	return 0;
}
int open_f(char *name, const char* mode)
{
	std::cout << "open\n";
	return 0;
}
int close_f(char *name)
{
	std::cout << "close\n";
	return 0;
}
int write_f(char *name, char *buffer, int length)
{
	std::cout << "write\n";
	return 0;
}
int read_f(char *name, char *buffer, int length)
{
	std::cout << "read\n";
	return 0;
}
char* pwd()
{
	std::cout << "pwd\n";
	return NULL;
}
int login(char *user_name, char *passwd)
{
	std::cout << "login\n";
	return 0;
}
int logout(char *user_name)
{
	std::cout <<"logout\n";
	return 0;
}
char* cat(char *file_name)
{
	std::cout <<"cat\n";
	return NULL;
}
void show_help_info()
{
	std::cout << "ls mkdir rmdir chdir create delete open close write read quit login logout pwd cat\n";
}

