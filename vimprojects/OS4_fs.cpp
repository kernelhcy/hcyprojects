#include "headers.h"
#include "fs.h"
#include "fs_structs.h"

//函数声明
static int init();
static int read_fs();
static int format_fs();
static int find_usr(char *username, struct user *usr);
static int create_user(char *username, char *passwd);
static int parse_dir_path(const char * path);
static int diralloc();
static int dirfree(int id);
static int ialloc();
static int ifree(int id);
static int balloc();
static int bfree(int id);
static int access(int inode_id);
static int find(const char *name);

//全局变量定义
static struct supernode g_sn;			//超级块
static struct dir_info g_dir_info;		//目录信息
static struct imap g_imap;				//inode位图
static struct bmap g_bmap;				//物理块位图

static struct inode *head = NULL;		//i内存节点链表头


static struct user login_users[MAX_LOGIN_USR];				//登录的用户列表
static struct user_ofile user_ofile_table[MAX_LOGIN_USR]; 	//用户打开文件列表
static int usr_num = 0;										//登陆的用户个数

static struct system_ofile system_ofile_table;				//系统打开的文件列表

static char tip[50];										//命令提示
static int curr_usr_id;										//当前登录用户在用户列表中的位置。

static union block blocks[BNODE_SIZE];						//模拟物理块
static struct directory dir_table[DIR_NUM];					//模拟目录表
static struct dinode dinodes[INODE_SIZE];					//模拟硬盘i节点
static struct inode inodes[INODE_SIZE];						//内存i节点

static char* curr_path;										//当前工作目录
static unsigned int curr_dir_id = 0;						//当前工作目录的id号


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
			ls(NULL);
		}
		else if(strcmp("mkdir", cmd) == 0)
		{
			printf("目录名：");
			char name[DIR_NAME_SIZE];
			scanf("%s",name);
			mkdir(name);
		}
		else if(strcmp("rmdir", cmd) == 0)
		{
			printf("删除目录。\n");
		}
		else if(strcmp("chdir", cmd) == 0 || strcmp("cd", cmd) == 0)
		{
			char path[300];
			printf("目录路径： ");
			scanf("%s",path);
			chdir(path);
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
						//printf("创建成功！\n登录系统...\n");
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
		printf("请重新启动。\n");
		exit(0);
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
 * 	文件系统的存储结构：
 *		---------------------------------------------------------------------------
 *      | 超级块 | inode位图 | 物理块位图 | 目录信息块 | 目录表  |  inode表  |    物理块             |
 *      ---------------------------------------------------------------------------
 * 目录表的第一项和inode的第一项被根目录占用
 */
static int format_fs()
{
	FILE *fd = NULL;
	//printf("fd :%d \n",fd);
	
	printf("创建文件系统...\n");
	
	fd = fopen("./fsdata","a+");
    
    if(NULL == fd)
    {
    	printf("文件系统创建失败！！\n");
    	return -1;
    }
    

	//初始化超级块
	g_sn.s_isize = INODE_SIZE;
	g_sn.s_bsize = BNODE_SIZE;
	g_sn.s_nfree = BNODE_SIZE;
	g_sn.s_pfree = 0;
	g_sn.s_ninode = INODE_SIZE;
	g_sn.s_pinode = 0;
	g_sn.s_rinode = 0;
	g_sn.s_fmod = 's';
	g_sn.disk_size = sizeof(struct supernode)
				  +sizeof(struct imap)
				  +sizeof(struct bmap)
				  +sizeof(struct dir_info)
				  +sizeof(struct directory)*DIR_NUM
				  +sizeof(struct dinode)*g_sn.s_isize
				  +sizeof(union block)*g_sn.s_bsize
				  ;
	fwrite(&g_sn, sizeof(struct supernode), 1, fd);
	
	//创建inode节点位图，并写入文件
	for(int i = 0; i < IMAP_SIZE; ++i)
	{
		g_imap.bits[i] = 0xffffffffffffffffLL;
	}
	//第一个inode被根目录占用。
	g_imap.bits[0] = 0x7fffffffffffffffLL;
	
	fwrite(&g_imap, sizeof(struct imap), 1, fd);
	
	//创建物理块位图，并写入文件。
	for(int i = 0; i < BMAP_SIZE; ++i)
	{
		g_bmap.bits[i] = 0xffffffffffffffffLL;
	}
	fwrite(&g_bmap, sizeof(struct bmap), 1, fd);

	//创建目录信息块，并写入文件
	g_dir_info.root_id = 0;
	g_dir_info.size = 1;//根目录存在
	
	//初始化位图
	for(int i = 0; i < DIR_BMAP_SIZE; ++i)
	{
		g_dir_info.bitmap[i] = 0xffffffffffffffffLL;
	}
	g_dir_info.bitmap[0] = 0x7fffffffffffffffLL;
	
	fwrite(&g_dir_info, sizeof(struct dir_info), 1, fd);


	//创建目录表，写入文件
	//设置根目录的目录名为root
	strcpy(dir_table[0].d_name, "root");
	dir_table[0].d_id = 0;
	//设置根目录的父目录为自己
	dir_table[0].parent_id = 0;
	dir_table[0].inode_id = 0;//inode为第一个。	
	dir_table[0].sub_cnt = 0;//子目录和文件的个数。
	
	memset(dir_table[0].file_inode, 0, DIR_INCLUDE_NUM);
	memset(dir_table[0].sub_dir_ids, 0, DIR_INCLUDE_NUM);
	
	fwrite(dir_table, sizeof(struct directory), DIR_NUM, fd);
	
	//创建inode并写入文件
	fwrite(dinodes, sizeof(struct dinode), INODE_SIZE, fd);

	//创建物理块，写入文件。
	fwrite(blocks, sizeof(union block), BNODE_SIZE, fd);
	
	fflush(fd);
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
	
	//读取i节点位图
	read_size = fread(&g_imap, sizeof(struct imap), 1, fd);
	if(read_size <= 0)
	{
		printf("读取i节点位图错误！！\n");
		return -1;
	}
	
	//读取物理块位图
	read_size = fread(&g_bmap, sizeof(struct bmap), 1, fd);
	if(read_size <= 0)
	{
		printf("读取物理块位图错误！！\n");
		return -1;
	}
	
	//读取目录信息块
	read_size = fread(&g_dir_info, sizeof(struct dir_info), 1, fd);
	if(read_size <= 0)
	{
		printf("读取目录信息块错误！！\n");
		return -1;
	}
	
	//读取目录表
	fread(dir_table, sizeof(struct directory),DIR_NUM, fd);
	//读取i节点表
	fread(dinodes, sizeof(struct dinode),INODE_SIZE, fd);
	//读取物理块
	fread(blocks, sizeof(union block),BNODE_SIZE, fd);	
	
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
			return 0;//找到！
		}
	}
	
	return -1;
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
	
	//printf("创建用户信息。\n");
	//创建用户
	struct user usr;
	usr.p_uid = (int)time(0);
	usr.p_gid = 1001 ;
	
	strcpy(usr.username, username);
	strcpy(usr.passwd,passwd);
	
	//printf("写入文件。\n");
	//写入文件。
	fwrite(&usr, sizeof(struct user), 1, fd);
	
	fclose(fd);
	
	printf("用户名：%s 密码：%s UID：%d GID：%d \n",username, passwd, usr.p_uid, usr.p_gid);
	
	//用户登录。
	login(username, passwd);
}

/*
 * 显示当前系统中的文件和目录。
 * id为要显示的目录的id。black为格式化显示计数器。
 */
static int show_names(int id,int black)
{
	
	
	for(int i = 0; i < black; ++i)
	{
		printf("    ");
	}
	printf("%c%c%c%s(%d) %d\n",3,6,6,dir_table[id].d_name, id, dir_table[id].sub_cnt);
	
	for(int i = 0; i < dir_table[id].sub_cnt ; ++i)
	{
		if(dir_table[id].sub_dir_ids[i] > 0)
		{
			
			show_names(dir_table[id].sub_dir_ids[i], black+1);
		}
	}
	
	return 0;
	
}

char * ls(char *path)
{
	//显示当前系统中的所有文件加和目录。
	show_names(0,0);
	
	return NULL;
}
int mkdir(char *name)
{
	
	int p_id;//父目录在目录表中的位置
	int s_id;//子目录在目录表中的位置
	
	int dir_id = diralloc();
	if(dir_id < 0)
	{
		printf("没有空间!!\n");
		return -1;
	}
	
	s_id = dir_id;
	p_id = curr_dir_id;
	
	//设置父目录里面关于子目录的信息。
	//子目录名字
	strcpy(dir_table[p_id].file_name[dir_table[p_id].sub_cnt], name);
	//子目录id
	dir_table[p_id].sub_dir_ids[dir_table[p_id].sub_cnt] = s_id;	
	//子目录个数增加
	++dir_table[p_id].sub_cnt;
	
	//设置子目录的信息。
	//目录名
	strcpy(dir_table[s_id].d_name, name);
	//id
	dir_table[s_id].d_id = s_id;
	//父目录id
	dir_table[s_id].parent_id = p_id;
	
	//给目录分分配inode节点
	int i_id = ialloc();
	if(i_id <= 0)
	{
		printf("i节点分配失败！\n");
		dirfree(s_id);//释放分配的目录项
		return -1;
	}
	dir_table[s_id].inode_id = i_id;
	//初始化i节点信息。
	dinodes[i_id].dir_or_file = DIR_T;
	dinodes[i_id].di_number = 0;
	//设置默认权限。
	dinodes[i_id].di_mode = 75;//rwxr-x：所有者具有所有权限，其他人只有查看的权限。
	//设置所有者的用户id和组id。
	dinodes[i_id].di_uid = login_users[usr_num].p_uid;
	dinodes[i_id].di_gid = login_users[usr_num].p_gid;
	
	/*
	 * 直接索引的第一个存放其目录表中的位置！下标。
	 */
	dinodes[i_id].direct_addr[0] = s_id;
	
	//清空子目录的子目录的信息。
	dir_table[s_id].sub_cnt = 0;
	memset(dir_table[s_id].file_inode, 0, DIR_INCLUDE_NUM);
	memset(dir_table[s_id].sub_dir_ids, 0, DIR_INCLUDE_NUM);
	
	return 0;
}

int rmdir(char *name)
{
	
	
	return 0;
}

int chdir(char *path)
{
	
	int dir_id = parse_dir_path(path);
	if(dir_id < 0)
	{
		printf("路径有错误！！\n");
		return -1;
	}
	
	printf("目录更改为：%s  id:%d\n",path, dir_id);
	curr_dir_id = dir_id;
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
	
	//printf("登录成功！\n");
	
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


/*
 * 拷贝字符串。
 * 从串src的begin开始，到end结束。拷贝到des中。des从0开始。
 * 不包括end出的字符！！
 */
static void str_cpy(char * des, const char * src, int begin, int end)
{
	int j = 0;
	for(int i = begin; i < end; ++i, ++j)
	{
		des[j] = src[i];
	}
	des[++j] = '\0';
	return;
}
/*
 * 解析目录路径。
 * 获得其目录id。 
 */
static int parse_dir_path(const char * path)
{
	char tmp_name[DIR_NAME_SIZE];	//当前正在解析的目录名
	int tmp_dir_id = 0;				//当前正在解析的目录号
	
	
	int index = 0;
	int begin,end;
	
	
	
	while(path[index] != '\0' && path[index] != '/')
	{
		++index;
	}
	
	while(path[index] != '\0')
	{
		++index;
	
		//获得目录名的开始和结束位置。
		begin = index ;
		while(path[index] != '\0' && path[index] != '/')
		{
			++index;
		}
		end = index;
		
		memset(tmp_name,'\0',DIR_NAME_SIZE);
		str_cpy(tmp_name, path, begin, end);
		
		int tmp = -1;
		
		//查看子目录中是否有此目录。
		for(int i = 0; i < dir_table[tmp_dir_id].sub_cnt; ++i)
		{
			if(strcmp(tmp_name, dir_table[tmp_dir_id].file_name[i]) == 0)
			{
				if(dir_table[tmp_dir_id].sub_dir_ids[i] != 0)//是目录（任何目录的子目录都不可能是根目录！）
				{
					tmp = i;
					break;
				}
				
			}
		}
		
		if(tmp < 0)//路径有错误。
		{
			return -1;
		}
		
		if(dir_table[tmp_dir_id].sub_dir_ids[tmp] != 0)
		{
			tmp_dir_id = dir_table[tmp_dir_id].sub_dir_ids[tmp];
		}
	}
	
	return tmp_dir_id;
}

/*
 * 在当前目录及子目录中搜索指定文件名的目录或文件。
 * 返回其i节点号。
 */
static int find(const char *name)
{
	
}


/*
 * 分配目录表
 * 返回分配的目录表项的索引。
 */
static int diralloc()
{
	 int index = -1;
	 
	 
	 while(index < DIR_BMAP_SIZE && g_dir_info.bitmap[++index] == 0);
	 
	 //没有空闲的空间
	 if(index >= DIR_BMAP_SIZE)
	 {
	 	return -1;
	 }
	 
	 unsigned long long test_b = 1;
	 int shift = 63;
	 while((g_dir_info.bitmap[index] & (test_b << shift)) == 0)
	 {
	 	--shift;
	 }
	 
	 //分配
	 g_dir_info.bitmap[index] = g_dir_info.bitmap[index] ^ (test_b << shift);
	 ++g_dir_info.size;//目录个数加一
	 
	 return index * 64 + (64 - shift) - 1;
	 
}

/*
 * 释放目录项
 */
static int dirfree(int id)
{
	int index = id / 64;
	int shift = id % 64;
	
	unsigned long long test_b = 1 << shift;
	g_dir_info.bitmap[index] = g_dir_info.bitmap[index] ^ test_b;
	
	//目录个数减一
	--g_dir_info.size;
	
	return 0;	
}

/*
 * 分配i节点
 */
static int ialloc()
{
	int index = -1;
	 
	 
	 while(index < IMAP_SIZE && g_imap.bits[++index] == 0);
	 
	 //没有空闲的空间
	 if(index >= IMAP_SIZE)
	 {
	 	return -1;
	 }
	 
	 unsigned long long test_b = 1;
	 int shift = 63;
	 while((g_imap.bits[index] & (test_b << shift)) == 0)
	 {
	 	--shift;
	 }
	 
	 //分配
	 g_imap.bits[index] = g_imap.bits[index] ^ (test_b << shift);
	 --g_sn.s_ninode;
	 
	 return index * 64 + (64 - shift) - 1;
}

/*
 * 释放i节点
 */
static int ifree(int id)
{
	int index = id / 64;
	int shift = id % 64;
	
	unsigned long long test_b = 1 << shift;
	g_imap.bits[index] = g_imap.bits[index] ^ test_b;
	
	++g_sn.s_ninode;
	
	return 0;
	
}

/*
 * 分配物理块
 */
static int balloc()
{
	int index = -1;
	 
	 
	 while(index < BMAP_SIZE && g_bmap.bits[++index] == 0);
	 
	 //没有空闲的空间
	 if(index >= BMAP_SIZE)
	 {
	 	return -1;
	 }
	 
	 unsigned long long test_b = 1;
	 int shift = 63;
	 while((g_bmap.bits[index] & (test_b << shift)) == 0)
	 {
	 	--shift;
	 }
	 
	 //分配
	 g_bmap.bits[index] = g_bmap.bits[index] ^ (test_b << shift);
	 --g_sn.s_nfree;
	 
	 return index * 64 + (64 - shift) - 1;
}

/*
 * 释放物理块
 */
static int bfree(int id)
{
	int index = id / 64;
	int shift = id % 64;
	
	unsigned long long test_b = 1 << shift;
	g_bmap.bits[index] = g_bmap.bits[index] ^ test_b;
	
	++g_sn.s_nfree;
	
	return 0;
	
}

/*
 * 访问控制函数。
 * 参数f_id为要访问的文件或目录的i节点号。
 *
 * 返回值： 
 *		R_R:1,读权限
 *		W_R:2,写权限
 *		E_R:4,运行权限
 *		或这三个值的任意组合
 */
static int access(int inode_id)
{
	int right = 0;
	
	return 0;
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
	
	//写回超级块
	fwrite(&g_sn, sizeof(struct supernode), 1, fd);
	
	//写回i节点位图
	fwrite(&g_imap, sizeof(struct imap), 1, fd);
	
	//写回物理块节点位图
	fwrite(&g_bmap, sizeof(struct bmap), 1, fd);

	//写回目录信息块
	fwrite(&g_dir_info, sizeof(struct dir_info), 1, fd);

	//写回目录表
	fwrite(dir_table, sizeof(struct directory), DIR_NUM, fd);
	
	//写回i节点
	fwrite(dinodes, sizeof(struct dinode), INODE_SIZE, fd);

	//写回物理块
	fwrite(blocks, sizeof(union block), BNODE_SIZE, fd);
	
	fflush(fd);
	fclose(fd);
	return 0;
}


void show_help_info()
{
	printf("\n  帮助信息。\n");

	printf("\tls\t显示当前目录的内容。\n\tmkdir\t创建目录。\n\trmdir\t删除目录。\n\tchdir\t更改当前工作目录。\n\tcreate\t创建文件。\n\tdelete\t删除文件。\n\topen\t打开文件。\n\tclose\t关闭文件。\n\twrite\t向文件中写数据。\n\tread\t读文件中的数据。\n\tquit\t退出。\n\tlogin\t用户登录。\n\tlogout\t用户登出。\n\tpwd\t显示当前工作目录。\n\tcat\t显示文件内容。\n");
	return ;
}
