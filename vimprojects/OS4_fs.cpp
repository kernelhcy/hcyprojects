#include "headers.h"
#include "fs.h"
#include "fs_structs.h"
int init();
int read_fs();
int create_fs();
/*
 * 程序入口
 */
void run(bool show_details)
{
	system("clear");//清屏

	printf("HCY's Fils System.\n");
	printf("Version 0.001. \n");
	
	int state = init();

	if(state == -1)//文件系统运行失败。
	{
		printf("文件系统出错！！退出...\n");
		return;
	}
	
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
	printf("初始化文件系统...\n");
	
	//打开文件
	FILE *fd = NULL;
	fd = fopen("./fsdata","r");
	
	int state;
	if(NULL == fd)
	{
		//创建文件系统。
		state = create_fs();
	}
	else
	{
		//加载文件系统。
		state = read_fs();
	}
	
	//文件系统运行失败。。。
	if(state == -1)
	{
		
		return -1;
	}
	
	return 0;
}

/*
 * 从硬盘中读去文件系统的信息。
 * 
 */
int read_fs()
{
	printf("文件系统加载中...\n");
	
	return 0;
	
}

/*
 * 创建文件系统。
 * 用于第一次使用文件系统，类似于格式化硬盘。
 */
int create_fs()
{
	printf("创建文件系统...\n");
	FILE *fd = NULL;
	fd = fopen("./fsdata","a+");
    
    if(NULL == fd)
    {
    	printf("文件系统创建失败！！\n");
    	return -1;
    }
    
	//创建超级块并写入文件系统。
	struct supernode sn;

	//初始化超级块
	sn.s_isize = INODE_SIZE;
	sn.s_bsize = BNODE_SIZE;
	sn.s_nfree = BNODE_SIZE;
	sn.s_pfree = 0;
	sn.s_ninode = INODE_SIZE;
	sn.s_pinode = 0;
	sn.s_rinode = 0;
	sn.s_fmod = 's';

	fwrite(&sn, sizeof(struct supernode), 1, fd);
	
	//创建inode节点位图，并写入文件
	struct imap i_map;
	for(int i = 0; i < IMAP_SIZE; ++i)
	{
		i_map.bits[i] = 0xffffffff;
	}
	fwrite(&i_map, sizeof(struct imap), 1, fd);

	//创建物理块位图，并写入文件。
	struct bmap b_map;
	for(int i = 0; i < BMAP_SIZE; ++i)
	{
		b_map.bits[i] = 0xffffffff;
	}
	fwrite(&b_map, sizeof(struct bmap), 1, fd);

	//创建inode并写入文件
	struct dinode node;
	node.di_number = 0;
	node.di_mode = 0;

	node.di_uid = -1;
	node.di_gid = -1;

	node.di_size = -1;
	for(int i = 0; i < NADDR; ++i)
	{
		node.di_addr[i] = -1;
	}
	
	for(int i = 0; i < INODE_SIZE; ++i)
	{
		fwrite(&node, sizeof(struct dinode), 1, fd);
	}

	//创建物理块，写入文件。
	struct block blo;
	for(int i = 0; i < BNODE_SIZE; ++i)
	{
		fwrite(&blo, sizeof(struct block), 1, fd);
	}

	fclose(fd);
	
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
	printf("\n  帮助信息。\n");

	printf("\tls\t显示当前目录的内容。\n\tmkdir\t创建目录。\n\trmdir\t删除目录。\n\tchdir\t更改当前工作目录。\n\tcreate\t创建文件。\n\tdelete\t删除文件。\n\topen\t打开文件。\n\tclose\t关闭文件。\n\twrite\t向文件中写数据。\n\tread\t读文件中的数据。\n\tquit\t退出。\n\tlogin\t用户登录。\n\tlogout\t用户登出。\n\tpwd\t显示当前工作目录。\n\tcat\t显示文件内容。\n");
	return ;
}


