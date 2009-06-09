#include "headers.h"
#include "fs.h"
#include "fs_structs.h"

//函数声明
static int init();

static int read_fs();

static int format_fs();

static int find_usr(char *username, struct user *usr);

static int create_user(char *username, char *passwd);

//全局变量定义
static struct supernode g_sn;			//超级块
static struct dir_table g_dir_table;	//目录表
static struct imap g_imap;				//inode位图
static struct bmap g_bmap;				//物理块位图

static struct inode *head = NULL;		//i内存节点链表头

static long inode_bpos = -1;			//i节点在文件中的开始位置
static long block_bpos = -1;			//物理块在文件中的开始位置
static long inode_pre = -1;				//指向i节	点的指针
static long block_pre = -1;				//指向物理块的指针

static struct user login_users[MAX_LOGIN_USR];				//登录的用户列表
static struct user_ofile user_ofile_table[MAX_LOGIN_USR]; 	//用户打开文件列表
static int usr_num = 0;										//登陆的用户个数

static struct system_ofile system_ofile_table;				//系统打开的文件列表

static char tip[50];										//命令提示
static int curr_usr_id;										//当前登录用户在用户列表中的位置。

static union block buffer[BUFFER_SIZE];						//内存中的缓冲区
/*
 * 程序入口
 */
void run(bool show_details)
{
	system("clear");//清屏
	
	
	//printf("int: %d char: %d\n",sizeof(int),sizeof(char));

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
	
	//设置命令提示。
	strcpy(tip, "cmd:");

	while(true)
	{
		printf("%s",tip);
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
			char username[USR_NAME_SIZE];
			char passwd[PWD_SIZE];
			struct user user_info;
			
			printf("用户名：");
			scanf("%s",username);
			if(find_usr(username, &user_info) < 0)
			{
				printf("用户不存在！！创建用户?y/n");
				char s[5];
				scanf("%s",s);
				if(s[0] == 'y')
				{
					printf("用户名：");
					scanf("%s",username);
					printf("口令：");
					scanf("%s",passwd);
					
					int state = create_user(username, passwd);
					
					if(state == 0)
					{
						printf("创建成功！\n登录系统...\n");
					}
					else
					{
						printf("创建失败...\n");
						break ;
					}
				}
			}
			else
			{
				printf("口令：");
				scanf("%s",passwd);
				login(username,passwd);
			}
			
		}
		else if(strcmp("logout", cmd) == 0)
		{
			logout(login_users[curr_usr_id].username);
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
	
	//退出文件系统，写回数据。
	halt();
}

/*
 * 初始化文件系统
 */
static int init()
{
	printf("初始化文件系统...\n");
	
	//打开文件
	FILE *fd = NULL;
	
	fd = fopen("./fsdata","r");
	
	
	int state;
	if(NULL == fd)
	{
		//创建文件系统。
		state = format_fs();
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
 * 创建文件系统。
 * 用于第一次使用文件系统，类似于格式化硬盘。
 */
static int format_fs()
{
	FILE *fd = NULL;
	//printf("fd :%d \n",fd);
	
	printf("创建文件系统...\n");
	
	fd = fopen("./fsdata","w+");
    
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
	
	//创建目录表，写入文件
	struct dir_table d_table;
	d_table.root_id = 0;
	d_table.size = 1;//根目录存在
	//设置根目录的目录名为root
	strcpy(d_table.dirs[0].d_name, "root");
	d_table.dirs[0].d_id = 0;
	//设置根目录的父目录为自己
	d_table.dirs[0].parent_id = 0;
	d_table.dirs[0].inode_id = 0;//inode为第一个。
	
	fwrite(&d_table, sizeof(struct dir_table), 1, fd);
	
	//创建inode节点位图，并写入文件
	struct imap i_map;
	for(int i = 0; i < IMAP_SIZE; ++i)
	{
		i_map.bits[i] = 0xffffffffffffffffLL;
	}
	//第一个inode被根目录占用。
	i_map.bits[0] = 0x7fffffffffffffffLL;
	
	
	fwrite(&i_map, sizeof(struct imap), 1, fd);

	//创建物理块位图，并写入文件。
	struct bmap b_map;
	for(int i = 0; i < BMAP_SIZE; ++i)
	{
		b_map.bits[i] = 0xffffffffffffffffLL;
	}
	fwrite(&b_map, sizeof(struct bmap), 1, fd);

	//创建inode并写入文件
	struct dinode node;
	node.di_number = 0;
	node.di_mode = 0;

	node.di_uid = -1;
	node.di_gid = -1;

	node.di_size = -1;
	
	for(int i = 0; i < INODE_SIZE; ++i)
	{
		fwrite(&node, sizeof(struct dinode), 1, fd);
	}

	//创建物理块，写入文件。
	union block blo;
	for(int i = 0; i < BNODE_SIZE; ++i)
	{
		fwrite(&blo, sizeof(union block), 1, fd);
	}

	fclose(fd);
	
	return 0;
}


/*
 * 从硬盘中读去文件系统的信息。
 * 
 */
static int read_fs()
{
	printf("文件系统加载中...\n");
	FILE *fd = NULL;
	fd = fopen("./fsdata","r");
	
	if(NULL == fd)
	{
		printf("文件系统加载错误！！\n");
		return -1;
	}
	
	int pos = 0;//当前文件位置指针
	
	int read_size = -1;
	
	//读取超级块 
	read_size = fread(&g_sn, sizeof(struct supernode), 1, fd);
	if(read_size <= 0)
	{
		printf("读取超级块出错！！\n");
		return -1;
	}
	pos += read_size;
	
	//读取目录表
	read_size = fread(&g_dir_table, sizeof(struct dir_table), 1, fd);
	if(read_size <= 0)
	{
		printf("读取目录表错误！！\n");
		return -1;
	}
	pos += read_size;
	
	//读取i节点位图
	read_size = fread(&g_imap, sizeof(struct imap), 1, fd);
	if(read_size <= 0)
	{
		printf("读取i节点位图错误！！\n");
		return -1;
	}
	pos += read_size;
	
	//读取物理块位图
	read_size = fread(&g_bmap, sizeof(struct bmap), 1, fd);
	if(read_size <= 0)
	{
		printf("读取i节点位图错误！！\n");
		return -1;
	}
	pos += read_size;
	
	//i节点的开始位置。
	inode_bpos = pos;
	inode_pre = inode_bpos;
	//计算物理块的开始位置。
	block_bpos = pos + (sizeof(struct dinode)*g_sn.s_isize);
	block_pre = block_bpos;
	
	printf("Inode_Begin_pos: %d\n",inode_bpos);
	printf("Inode_pre: %d\n",inode_pre);
	printf("Block_Begin_pos: %d\n",block_bpos);
	printf("Block_pre: %d\n",block_pre);
	
	fclose(fd);//关闭文件
	
	return 0;
	
}



/*
 * 查找用户是否存在。
 * 如存在，则返回0并填充用户信息到参数usr中，否则返回-1.
 */
static int find_usr(char *username, struct user *usr)
{
	FILE *fd = NULL;
	fd = fopen("./users", "r");
	
	if(NULL == fd)//用户信息文件不存在
	{
		return -1;
	}
	
	//printf("find_usr:打开文件。\n");
	int tmp_s;
	
	tmp_s = fread(usr, sizeof(struct user), 1, fd);
	
	//printf("find_usr: %s\n",usr->username);
	
	if(strcmp(usr -> username, username) == 0)
	{
		return 0;
	}
	//搜寻文件中的所有用户
	while(tmp_s > 0)
	{
		tmp_s = fread(usr, sizeof(struct user), 1, fd);
		
		//printf("find_usr: %s\n",usr->username);
		
		if(strcmp(usr -> username, username) == 0)
		{
			return 0;
		}
	}
	
	return NULL;
}

/*
 * 创建用户。
 */
static int create_user(char *username, char *passwd)
{
	FILE *fd = NULL;
	fd = fopen("./users", "a+");
	
	if(NULL == fd)//打开文件错误
	{
		return -1;
	}
	
	printf("创建用户信息。\n");
	//创建用户
	struct user usr;
	usr.p_uid = 123;
	usr.p_gid = 123;
	
	strcpy(usr.username, username);
	strcpy(usr.passwd,passwd);
	
	printf("写入文件。\n");
	//写入文件。
	fwrite(&usr, sizeof(struct user), 1, fd);
	
	fclose(fd);
	
	//用户登录。
	login(username, passwd);
}

char * ls(char *path)
{
	printf("ls\n");
	return 0;
}
int mkdir(char *path)
{
	printf("mkdir\n");
	return 0;
}
int rmdir(char *path)
{
	printf("rmdir\n");
	return 0;
}
int chdir(char *path)
{
	printf("chdir\n");
	return 0;
}
int create_f(char *name, int mode)
{
	printf("create\n");
	return 0;
}
int delete_f(char *name)
{
	printf("delete\n");
	return 0;
}
int open_f(char *name, const char* mode)
{
	printf("open\n");
	return 0;
}
int close_f(char *name)
{
	printf("close\n");
	return 0;
}
int write_f(char *name, char *buffer, int length)
{
	printf("write\n");
	return 0;
}
int read_f(char *name, char *buffer, int length)
{
	printf("read\n");
	return 0;
}
char* pwd()
{
	printf("pwd\n");
	return NULL;
}

int login(char *username, char *passwd)
{

	struct user user_info;
	
	if(find_usr(username, &user_info) < 0)
	{
		printf("用户不存在！！\n");
		return -1;
	}
	
	if(strcmp(user_info.passwd, passwd) != 0)
	{
		printf("密码错误。\n");
		return -1;
	}
	
	//将用户信息拷贝到登录用户列表中。
	memcpy(&login_users[usr_num], &user_info, sizeof(struct user));
	curr_usr_id = usr_num;
	++usr_num;//更新登录用户个数
	strcpy(tip, user_info.username);
	strcat(tip, "@cmd:");
	
	printf("登录成功！\n");
	
	return 0;
}


int logout(char *user_name)
{
	--usr_num;
	strcpy(tip, "cmd:");
	return 0;
}
char* cat(char *file_name)
{
	printf("cat\n");
	return NULL;
}

int halt()
{
	FILE * fd = NULL;
	fd = fopen("./fsdata", "w+");
	
	if(NULL == fd)
	{
		printf("对文件系统的修改未能写回文件中！\n");
		return -1;
	}
	
	int pos = 0;
	
	//写回超级块
	pos = fwrite(&g_sn, sizeof(struct supernode), 1, fd);

	//写回目录表
	fwrite(&g_dir_table, sizeof(struct dir_table), 1, fd);
		
	//写回i节点位图
	fwrite(&g_imap, sizeof(struct imap), 1, fd);
	
	//写回物理块位图
	fwrite(&g_bmap, sizeof(struct bmap), 1, fd);
	
	
	fclose(fd);
	return 0;
}


void show_help_info()
{
	printf("\n  帮助信息。\n");

	printf("\tls\t显示当前目录的内容。\n\tmkdir\t创建目录。\n\trmdir\t删除目录。\n\tchdir\t更改当前工作目录。\n\tcreate\t创建文件。\n\tdelete\t删除文件。\n\topen\t打开文件。\n\tclose\t关闭文件。\n\twrite\t向文件中写数据。\n\tread\t读文件中的数据。\n\tquit\t退出。\n\tlogin\t用户登录。\n\tlogout\t用户登出。\n\tpwd\t显示当前工作目录。\n\tcat\t显示文件内容。\n");
	return ;
}


