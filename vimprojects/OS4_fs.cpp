#include "headers.h"
#include "fs.h"
#include "fs_structs.h"

//函数声明
static int init();
static int read_fs();
static int format_fs();
static int find_usr(char *username, struct user *usr);
static int create_user(char *username, char *passwd);
static int diralloc();
static int dirfree(int id);
static int ialloc();
static int ifree(int id);
static int balloc();
static int bfree(int id);
static int access(int inode_id);
static int find(const char *name);
static int update_curr_paht_name();
static int read_indirect_block(union block *addr_block, int len, char* buffer);
static int write_block(int b_id, char *buffer, int size);
static void str_cpy(char * des, const char * src, int begin, int size);

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

static char curr_path[500];									//当前工作目录
static unsigned int curr_dir_id = 0;						//当前工作目录的id号

static FILE_P *t_fp = NULL;//调试使用
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
	char path[500];//存放临时的目录或文件名	
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
			scanf("%s",path);
			mkdir(path);
		}
		else if(strcmp("rmdir", cmd) == 0)
		{
			printf("目录名称：");
			scanf("%s",path);
			rmdir(path);
		}
		else if(strcmp("chdir", cmd) == 0 || strcmp("cd", cmd) == 0)
		{
			printf("完整的目录路径： ");
			scanf("%s",path);
			chdir(path);
		}
		else if(strcmp("create", cmd) ==0 || strcmp("crt", cmd) == 0)
		{
			printf("文件名： ");
			scanf("%s",path);
			t_fp = create_f(path,75);
		}
		else if(strcmp("delete", cmd) == 0 
					|| strcmp("del", cmd) == 0 || strcmp("rm", cmd) == 0)
		{
			printf("完整的路径和文件名： ");
			scanf("%s",path);
			delete_f(path);
		}
		else if(strcmp("open", cmd) == 0)
		{
			printf("完整的路径和文件名： ");
			scanf("%s", path);
			t_fp = open_f(path,R|W);
		}
		else if(strcmp("close", cmd) == 0)
		{
			close_f(t_fp);
		}
		else if(strcmp("write", cmd) == 0 || strcmp("wr", cmd) == 0)
		{
			char buffer[10000];
			memset(buffer, '\0', 10000);
			printf("内容:");
			scanf("%s", buffer);
			write_f(t_fp, buffer, strlen(buffer));
		}
		else if(strcmp("read", cmd) == 0 || strcmp("rd", cmd) == 0)
		{
			printf("读文件中的数据。\n");
		}
		else if(strcmp("pwd", cmd) == 0)
		{
			pwd();
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
		 	char *content = cat(t_fp);
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
		//printf("请重新启动。\n");
		//exit(0);
	}
	else
	{
		//加载文件系统。
		state = read_fs();
	}
	
	//设置当前工作目录
	curr_dir_id = 0;
	strcpy(curr_path, "/");

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
	g_imap.bits[IMAP_SIZE - 1] = 0x7fffffffffffffffLL;
	
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
	for(int i = 0; i < DMAP_SIZE; ++i)
	{
		g_dir_info.dmap[i] = 0xffffffffffffffffLL;
	}
	g_dir_info.dmap[DMAP_SIZE-1] = 0x7fffffffffffffffLL;
	
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
	printf("%c%c%c%s(d,%d) %d\n",3,6,6,dir_table[id].d_name, id, dir_table[id].sub_cnt);
	
	for(int i = 0; i < dir_table[id].sub_cnt ; ++i)
	{
		if(dir_table[id].sub_dir_ids[i] > 0)
		{
			
			show_names(dir_table[id].sub_dir_ids[i], black+1);
		}
		
		dinode *fi = NULL;
		fi = &dinodes[dir_table[id].file_inode[i]]; 
		if(fi -> dir_or_file == FILE_T)
		{
			for(int j = 0; j < black + 1; ++j)
			{
				printf("    ");
			}
			printf("%c%c%c%s(f,%d) %d\n",3,6,6,dir_table[id].file_name[i], dir_table[id].file_inode[i], fi -> di_size);
		}

	}
	
	return 0;
	
}

char * ls(char *path)
{

	printf("\n名称（文件(f)或目录(d), 目录ID或文件inode的id） 文件长度或目录容量\n\n");

	//显示当前系统中的所有文件加和目录。
	show_names(0,0);
	
	return NULL;
}
int mkdir(char *name)
{
	
	int p_id;//父目录在目录表中的位置
	int s_id;//子目录在目录表中的位置
	
	int dir_id = diralloc();
	printf("mkdir dir_id:%d\n", dir_id);
	if(dir_id < 0)
	{
		printf("没有空间!!\n");
		return -1;
	}
	
	//给目录分分配inode节点
	int i_id = ialloc();
	if(i_id <= 0)
	{
		printf("i节点分配失败！\n");
		dirfree(s_id);//释放分配的目录项
		return -1;
	}

	s_id = dir_id;
	p_id = curr_dir_id;
	
	//设置父目录里面关于子目录的信息。
	//子目录名字
	strcpy(dir_table[p_id].file_name[dir_table[p_id].sub_cnt], name);
	//子目录id
	dir_table[p_id].sub_dir_ids[dir_table[p_id].sub_cnt] = s_id;	
	//增加父目录中此子目录的i节点号
	dir_table[p_id].file_inode[dir_table[p_id].sub_cnt] = i_id;	
	//父目录中子目录个数增加
	++dir_table[p_id].sub_cnt;
	//printf("sub_cnt: %d\n",dir_table[p_id].sub_cnt);

	//设置子目录的信息。
	//目录名
	strcpy(dir_table[s_id].d_name, name);
	//子目录自己的目录号
	dir_table[s_id].d_id = s_id;
	//设置子目录的i节点号
	dir_table[s_id].inode_id = i_id;
	//设置子目录的父目录id
	dir_table[s_id].parent_id = p_id;

	//初始化i节点信息。
	dinodes[i_id].dir_or_file = DIR_T;
	//关联个数
	dinodes[i_id].di_number = 0;
	//设置默认权限。
	dinodes[i_id].di_right = 75;//rwxr-x：所有者具有所有权限，其他人只有查看的权限。
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
	
	int i_id = find(name);	

	//printf("id: %d\n",dir_id);
	
	if(i_id == 0)
	{
		printf("不能删除根目录！！\n");
		return -1;
	}
	
	if(i_id < 0 )
	{
		printf("目录不存在。");
		return -1;
	}

	int dir_id = dinodes[i_id].direct_addr[0];//获取目录id;

	//删除父目录中有关该子目录的信息。
	int p_id = dir_table[dir_id].parent_id;	

	//printf("Pid: %d\n",p_id);

	for(int i = 0; i < dir_table[p_id].sub_cnt; ++i)
	{
		//printf("%d ",dir_table[p_id].sub_dir_ids[i]);
		if(dir_table[p_id].sub_dir_ids[i] == dir_id)
		{
			//收缩。
			//printf("收缩。%d\n",i);
			for(int j = i+1; j < dir_table[p_id].sub_cnt; ++j)
			{
				dir_table[p_id].sub_dir_ids[j-1] = dir_table[p_id].sub_dir_ids[j];
				dir_table[p_id].file_inode[j-1] = dir_table[p_id].file_inode[j];
				strcpy(dir_table[p_id].file_name[j-1], dir_table[p_id].file_name[j]);
			}

			break;
		}
	}
	//子目录个数和索引减一。
	--dir_table[p_id].sub_cnt;

	//删除目录表中该目录的信息;
	dirfree(dir_id);

	return 0;
}


/*
 * 下面的update_curr_path_name方法的辅助方法。
 * 拷贝目录名curr_path.
 */
static int cpy_name_update(int id)
{

	//根目录目录名为“/”
	if(id == 0)
	{
		strcat(curr_path, "/");
		return 0;
	}

	//到达根目录的子目录，不再回溯根目录
	if(dir_table[id].parent_id == 0)
	{
		strcat(curr_path, "/");
		strcat(curr_path, dir_table[id].d_name);
		//printf("cpy_name_update: %s\n",dir_table[id].d_name);
		return 0;
	}

	//递归的复制父目录的父目录
	cpy_name_update(dir_table[id].parent_id);

	//复制当前目录
	strcat(curr_path, "/");
	strcat(curr_path, dir_table[id].d_name);
	//printf("cpy_name_update: %s\n",dir_table[id].d_name);

	return 0;
}
/*
 * 更新当前工作目录的目录路径名
 * 也就是更新变量curr_path
 */
static int update_curr_path_name()
{
	
	memset(curr_path, '\0', sizeof(curr_path));
	//curr_path[0] = '/';

	cpy_name_update(curr_dir_id);

	return 0;	
}


int chdir(char *path)
{
	//切换到根目录
	if(strlen(path) == 1 && path[0] == '/')
	{
		curr_dir_id = 0;
		update_curr_path_name();
		return 0;
	}

	int i_id = find(path);
	//printf("chdir: i_id: %d path: %s\n",i_id, path);
	if(i_id < 0)
	{
		printf("路径有错误！！\n");
		return -1;
	}
	
	int dir_id = dinodes[i_id].direct_addr[0];

	printf("目录更改为：%s  id:%d\n",path, dir_id);
	curr_dir_id = dir_id;
	//strcpy(curr_path, path);
	update_curr_path_name();

	return 0;
}

FILE_P* create_f(char *name, int right)
{
	
	int p_id;//父目录在目录表中的位置
	FILE_P *fp = NULL;
	
	//分配硬盘i节点
	int i_id = ialloc();
	if(i_id < 0)
	{
		printf("没有空间!!\n");
		return fp;
	}
	
	p_id = curr_dir_id;
	
	//设置父目录里面关于子文件的信息。
	//文件名字
	strcpy(dir_table[p_id].file_name[dir_table[p_id].sub_cnt], name);
	//文件inode的id
	dir_table[p_id].file_inode[dir_table[p_id].sub_cnt] = i_id;	
	//子文件和目录个数增加
	++dir_table[p_id].sub_cnt;

	//printf("sub_cnt: %d\n",dir_table[p_id].sub_cnt);
	//初始化i节点信息。
	fp = (FILE_P*)malloc(sizeof(struct inode));
	fp -> dir_or_file = FILE_T;
	fp -> di_number = 0;
	//设置默认权限。
	fp -> di_right = 75;
	//设置所有者的用户id和组id。
	fp -> di_uid = login_users[usr_num].p_uid;
	fp -> di_gid = login_users[usr_num].p_gid;
	//父目录的目录号
	fp -> parent_id = curr_dir_id;
	
	//设置硬盘i节点号。
	fp -> i_into = i_id;
	
	//引用个数
	fp -> i_count = 1;
	
	//管理个数
	fp -> di_number = 1;
	
	return fp;
}

int delete_f(char *name)
{
	
	int i_id = find(name);	
	//printf("delete id: %d\n",i_id);
		
	if(i_id <= 0 )
	{
		printf("文件不存在。");
		return -1;
	}

	//删除父目录中有关该子目录的信息。
	int p_id = dinodes[i_id].parent_id;	

	//printf("Pid: %d\n",p_id);

	for(int i = 0; i < dir_table[p_id].sub_cnt; ++i)
	{
		//printf("%d ",dir_table[p_id].file_inode[i]);
		if(dir_table[p_id].file_inode[i] == i_id)
		{
			//收缩。
			//printf("收缩。%d\n",i);
			for(int j = i+1; j < dir_table[p_id].sub_cnt; ++j)
			{
				dir_table[p_id].sub_dir_ids[j-1] = dir_table[p_id].sub_dir_ids[j];
				dir_table[p_id].file_inode[j-1] = dir_table[p_id].file_inode[j];
				strcpy(dir_table[p_id].file_name[j-1], dir_table[p_id].file_name[j]);
			}

			break;
		}
	}
	//子目录个数和索引减一。
	--dir_table[p_id].sub_cnt;


	//释放物理块空间。

	//释放i节点。
	ifree(i_id);

	return 0;
	return 0;
}

FILE_P* open_f(char *name, int mode)
{
	int i_id = find(name);
	FILE_P *fp = NULL;
	
	if(i_id <= 0)
	{
		printf("文件不存在!!\n");
		return NULL;
	}
	
	//分配内存i节点
	fp = (FILE_P*)malloc(sizeof(struct inode));
	
	//复制硬盘i节点的信息到内存i节点
	fp -> dir_or_file = dinodes[i_id].dir_or_file;
	fp -> di_number = dinodes[i_id].di_number;
	fp -> di_right = dinodes[i_id].di_right;
	fp -> di_uid = dinodes[i_id].di_uid;
	fp -> di_gid = dinodes[i_id].di_gid;
	fp -> parent_id = dinodes[i_id].parent_id;
	fp -> di_size = dinodes[i_id].di_size;
	memcpy(fp -> direct_addr,dinodes[i_id].direct_addr,sizeof(dinodes[i_id].direct_addr));
	fp -> addr = dinodes[i_id].addr;
	fp -> sen_addr = dinodes[i_id].sen_addr;
	fp -> tru_addr = dinodes[i_id].tru_addr;
	
	//设置内存i节点的额外信息。
	//物理i节点号
	fp -> i_into = i_id;
	//引用个数
	fp -> i_count = 1;
	//打开方式
	fp -> mode = mode;
	
	return fp;
}

int close_f(FILE_P *fp)
{

	int i_id = fp -> i_into;
	
	//复制内存i节点的信息到硬盘i节点
	dinodes[i_id].dir_or_file = fp -> dir_or_file;
	dinodes[i_id].di_number = fp -> di_number;
	dinodes[i_id].di_right = fp -> di_right;
	dinodes[i_id].di_uid = fp -> di_uid;
	dinodes[i_id].di_gid = fp -> di_gid;
	dinodes[i_id].parent_id = fp -> parent_id;
	dinodes[i_id].di_size = fp -> di_size;
	memcpy(dinodes[i_id].direct_addr,fp -> direct_addr,sizeof(dinodes[i_id].direct_addr));
	dinodes[i_id].addr = fp -> addr;
	dinodes[i_id].sen_addr = fp -> sen_addr;
	dinodes[i_id].tru_addr = fp -> tru_addr;
	--fp -> i_count;

	//没有用户引用此文件，释放内存i节点
	if(fp -> i_count <= 0 )
	{
		free(fp);
		t_fp = NULL;
	}
	
	return 0;
}

/*
 * 将数据写到指定的块中。
 * b_id物理块的块号。
 * buffer数据缓冲区，存放要写入的数据
 * size写入的数据的长度。要小于等于buffer中的数据长度！！
 * 返回写入的数据长度。
 *
 */
static int write_block(int b_id, char *buffer, int size)
{
	if(b_id < 0 || buffer == NULL || size < 0)
	{
		return 0;
	}

	int index = 0;//位置
	while(index < BLOCK_SIZE && index < size)
	{
		blocks[b_id].entry[index] = buffer[index];
		++index;
	}

	return index;
}

int write_f(FILE_P *fp, char *buffer, int length)
{
	printf("buffer: %d  %s\n",length ,buffer);
	
	int len = 0;			//已经写入文件的数据的长度。
	int b_id = -1;
	int buffer_index = 0;	//即将写入文件的数据的开始位置。
	
	char tmp_buffer[BLOCK_SIZE];//缓冲区。

	//直接地址
	printf("直接：\n");
	for(int i = 0; i < D_ADDR_NUM && len < length; ++i)
	{
		b_id = balloc();	//分配物理块
		printf("\tblock id %d\t", b_id);

		if(b_id < 0)
		{
			printf("无法分配物理块！！\n");
			return -1;
		}
		fp -> direct_addr[i] = b_id;

		if(length - len > BLOCK_SIZE)
		{
			//static void str_cpy(char * des, const char * src, int begin, int end)
			str_cpy(tmp_buffer, buffer, buffer_index,  BLOCK_SIZE);
			buffer_index += BLOCK_SIZE;
		}
		else
		{
			str_cpy(tmp_buffer, buffer, buffer_index, length - len);
			buffer_index = length - 1;
		}
		printf("write %d %s\t", buffer_index, tmp_buffer);
		len += write_block(b_id, tmp_buffer, BLOCK_SIZE);
		printf("len: %d\n", len);
	}
	
	printf("\n");
	//一级间接索引
	printf("一级：\n");
	b_id = balloc();	//分配一级索引地址物理块
	printf("\tblock id %d\n", b_id);

	if(b_id < 0)
	{
		printf("无法分配物理块！！\n");
		return -1;
	}

	fp -> addr = b_id;
	for(int i = 0; i < B_ADDR_NUM; ++i)
	{
		
		b_id = balloc();	//分配物理块
		printf("\t\tblock id %d\t", b_id);

		if(b_id < 0)
		{
			printf("无法分配物理块！！\n");
			return -1;
		}

		blocks[fp -> addr].b_addr[i] = b_id;

		if(length - len > BLOCK_SIZE)
		{
			str_cpy(tmp_buffer, buffer, buffer_index,  BLOCK_SIZE);
			buffer_index += BLOCK_SIZE;
		}
		else
		{
			str_cpy(tmp_buffer, buffer, buffer_index, length - len);
			buffer_index = length - 1;
		}
		printf("write %d %s\t", buffer_index, tmp_buffer);
		len += write_block(b_id, tmp_buffer, BLOCK_SIZE);
		printf("len: %d\n", len);
	}
	
	printf("\n");

	//二级间接索引
	printf("二级：\n");
	b_id = balloc();	//分配二级索引地址物理块
	printf("\tblock id %d\n", b_id);

	if(b_id < 0)
	{
		printf("无法分配物理块！！\n");
		return -1;
	}

	fp -> sen_addr = b_id;
	for(int i = 0; i < B_ADDR_NUM && len < length; ++i)
	{
		
		b_id = balloc();	//分配一级索引地址物理块
		printf("\t\tblock id %d\n", b_id);

		if(b_id < 0)
		{
			printf("无法分配物理块！！\n");
			return -1;
		}
		
		blocks[fp -> sen_addr].b_addr[i] = b_id;
		
		for(int j = 0; j < B_ADDR_NUM && len < length ; ++j)
		{
			
			b_id = balloc();	//分配物理块
			printf("\t\t\tblock id %d\t", b_id);
	
			if(b_id < 0)
			{
				printf("无法分配物理块！！\n");
				return -1;
			}
		
			blocks
				[
					blocks
					[
						fp -> sen_addr
					].b_addr[i]
				].b_addr[j] = b_id;
			
			if(length - len > BLOCK_SIZE)
			{
				str_cpy(tmp_buffer, buffer, buffer_index,  BLOCK_SIZE);
				buffer_index += BLOCK_SIZE;
			}
			else
			{
				str_cpy(tmp_buffer, buffer, buffer_index, length - len);
				buffer_index = length - 1;
			}
			printf("write %d %s\t", buffer_index, tmp_buffer);
			len += write_block(b_id, tmp_buffer, BLOCK_SIZE);
			printf("len: %d\n", len);
		}
	}

	printf("\n");

	//三级间接索引
	printf("三级：\n");
	b_id = balloc();	//分配三级索引地址物理块
	printf("\tblock id %d\n", b_id);

	if(b_id < 0)
	{
		printf("无法分配物理块！！\n");
		return -1;
	}

	fp -> tru_addr = b_id;
	for(int i = 0; i < B_ADDR_NUM && len < length; ++i)
	{
		
		b_id = balloc();	//分配二级索引地址物理块
		printf("\t\tblock id %d\n", b_id);

		if(b_id < 0)
		{
			printf("无法分配物理块！！\n");
			return -1;
		}
		
		blocks[fp -> tru_addr].b_addr[i] = b_id;
		
		for(int j = 0; j < B_ADDR_NUM && len < length ; ++j)
		{
			
			int tmp_id = balloc();	//分配一级索引地址物理块
			printf("\t\t\tblock id %d\n", b_id);
	
			if(b_id < 0)
			{
				printf("无法分配物理块！！\n");
				return -1;
			}
		
			blocks[b_id].b_addr[j] = tmp_id;
			for(int k = 0; k < B_ADDR_NUM && len < length; ++k)
			{
				int tmp_tmp_id = balloc();//分配物理块
				printf("\t\t\t\tblock id %d\t", b_id);
	
				if(b_id < 0)
				{
					printf("无法分配物理块！！\n");
					return -1;
				}

				blocks[tmp_id].b_addr[k] = tmp_tmp_id;
				if(length - len > BLOCK_SIZE)
				{
					str_cpy(tmp_buffer, buffer, buffer_index,  BLOCK_SIZE);
					buffer_index += BLOCK_SIZE;
				}
				else
				{
					str_cpy(tmp_buffer, buffer, buffer_index, length - len);
					buffer_index = length - 1;
				}
				printf("write %d %s\t", buffer_index, tmp_buffer);
				len += write_block(tmp_tmp_id, tmp_buffer, BLOCK_SIZE);
				printf("len: %d\n", len);
			}
		}
	}

	//设定文件程度
	fp -> di_size += length; 

	return 0;
}
int read_f(char *name, char *buffer, int length)
{
	printf("read\n");
	return 0;
}
char* pwd()
{
	//printf("当前目录号：%d 目录名称：%s\n", curr_dir_id, curr_path);
	printf("%s\n", curr_path);
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

char* cat(FILE_P *fp)
{
	if(NULL == fp)
	{
		printf("没有打开的文件!\n");
		return NULL;
	}

	int file_len = fp -> di_size;//文件的总长度
	int len = 0; //读取的数据长度

	//使用一级间接索引时，所等存储的最大数据量。
	int max_size = BLOCK_SIZE * B_ADDR_NUM;
	
	printf("文件长度：%d inode: %d\n", file_len, fp -> i_into);

	//读直接索引
	printf("直接索引块\n");
	for(int i = 0; i < D_ADDR_NUM && len < file_len; ++i)
	{
		for(int j = 0; j < BLOCK_SIZE && len < file_len; ++j)
		{
			printf("%c",blocks[fp -> direct_addr[i]].entry[j]);
			++len;
		}
		printf("\n");
	}
	//读间接索引
	
	if(len >= file_len)
	{
		return NULL;
	}

	char buffer[max_size];//缓冲区
	int addr = -1;//物理块地址

	//一级间接索引
	memset(buffer, '\0', max_size);
	len += read_indirect_block(&blocks[fp -> addr], max_size, buffer);
	printf("一级间接索引\n%s\n",buffer);

	//二级间接索引
	if(len >= file_len)
	{
		return NULL;
	}
	printf("二级间接索引 id: %d\n",fp -> sen_addr);
	for(int i = 0; i < B_ADDR_NUM && len < file_len; ++i)
	{
		memset(buffer, '\0', max_size);
		//printf("%d\n",blocks[dinodes[i_id].sen_addr].b_addr[i]);
		len += read_indirect_block(&blocks
									[
										blocks
										[
											fp -> sen_addr			//二级索引地址
										].b_addr[i]					//直接地址
									],
								max_size, buffer);
		printf("%s",buffer);
	}
		
	//三级间接索引
	if(len >= file_len)
	{
		return NULL;
	}
	printf("三级间接索引\n");
	union block *tmp_block;
	for(int i = 0; i < B_ADDR_NUM && len < file_len; ++i)
	{
		tmp_block = &blocks[
							    blocks
								[
									fp -> tru_addr			//三级索引地址
								].b_addr[i]					//二级间接地址
						   ];
		for(int j = 0; j < B_ADDR_NUM && len < file_len; ++j)
		{
			memset(buffer, '\0', max_size);
			len += read_indirect_block(&blocks[tmp_block -> b_addr[j]],
								max_size, buffer);
			printf("%s",buffer);
		}
		printf("len %d filelen %d\n", len, file_len);
	}
	
	return NULL;
}

/*
 * 读间接索引块
 * addr_block是存有间接索引地址的物理块的指针。
 * len是要读取的数据的长度。
 * buffer为存放内容的缓冲区。
 *  注：
 *  	缓冲区的大小必须小于等于len！！
 * 返回读取的数据长度，也即buffer中存放的读取的数据的长度。
 *
 */
static int read_indirect_block(union block *addr_block, int len, char* buffer)
{
	int addr = -1; //物理块地址。
	int length = 0;//读取的数据长度。

	if(addr_block == NULL || buffer == NULL)
	{
		return 0;
	}
	
	//读取数据
	for(int i = 0; i < B_ADDR_NUM; ++i)
	{
		addr = addr_block -> b_addr[i]; //获取地址
		//printf("r_i_b: addr: %d\n", addr);	
		//讲数据拷贝到buffer中。
		for(int j = 0; j < BLOCK_SIZE; ++j)
		{
			buffer[length] = blocks[addr].entry[j];
			++length;
			if(len == length)//读取了所需的长度
			{	
				return len;
			}
		}
	}
	return length;
}
/*
 * 拷贝字符串。
 * 从串src的begin开始，拷贝size个字到des中。des从0开始。
 * 不包括end出的字符！！
 */
static void str_cpy(char * des, const char * src, int begin, int size)
{
	int j = 0;
	for(int i = begin; j < size ; ++i, ++j)
	{
		des[j] = src[i];
	}
	des[++j] = '\0';
	return;
}
/*
 * 解析目录路径，查找文件和目录。
 * 获得其inode号。 
 * 当路径有错误时，返回-1，否则返回inode号。
 */
static int find(const char * path)
{
	char tmp_name[DIR_NAME_SIZE];	//当前正在解析的文件名
	int i_id = 0;					//当前正在解析的文件inode号
	int tmp_dir_id = 0;             //若当前解析的名称是目录的名称，记录其目录号。	
	int result = -1;				//最终的结果

	int index = 0;
	int begin,end;
	
	
	
	while(path[index] != '\0' && path[index] != '/')
	{
		++index;
	}
	
	while(path[index] != '\0')
	{

		++index;
	
		//获得文件名的开始和结束位置。
		begin = index ;
		while(path[index] != '\0' && path[index] != '/')
		{
			++index;
		}
		end = index;
		
		memset(tmp_name,'\0',DIR_NAME_SIZE);
		str_cpy(tmp_name, path, begin, end - begin);
		//printf("pares: %s\n",tmp_name);	
		int tmp = -1;
		
		//查看子目录中是否有此文件。
		//从根目录开始搜索。
		for(int i = 0; i < dir_table[tmp_dir_id].sub_cnt; ++i)
		{
			//printf("%s\n",dir_table[tmp_dir_id].file_name[i]);
			if(strcmp(tmp_name, dir_table[tmp_dir_id].file_name[i]) == 0)
			{
				tmp = i;
				break;
			}
		}
		
		if(tmp < 0)//路径有错误。
		{
			return -1;
		}
		
		if(dir_table[tmp_dir_id].file_inode[tmp] != 0)
		{
			i_id = dir_table[tmp_dir_id].file_inode[tmp];
			tmp_dir_id = dir_table[tmp_dir_id].sub_dir_ids[tmp];
		}
		result = i_id;
	}
	
	return result;
}



/*
 * 分配目录表
 * 返回分配的目录表项的索引。
 */
static int diralloc()
{
	 int index = DMAP_SIZE;
	 
	 
	 while(index >= 0 && g_dir_info.dmap[--index] == 0);
	 
	 //没有空闲的空间
	 if(index < 0)
	 {
	 	return -1;
	 }
	 
	 unsigned long long test_b = 1;
	 int shift = 63;
	 while((g_dir_info.dmap[index] & (test_b << shift)) == 0)
	 {
	 	--shift;
	 }
	 
	 printf("diralloc %llx\n", g_dir_info.dmap[index]);
	 
	 //分配
	 g_dir_info.dmap[index] = g_dir_info.dmap[index] ^ (test_b << shift);
	 ++g_dir_info.size;//目录个数加一
	 
	 printf("diralloc index %d shift %d\n",index, shift);
	 printf("diralloc %llx\n", g_dir_info.dmap[index]);
	 
	 return index * 64 + (64 - shift) - 1;
	 
}

/*
 * 释放目录项
 */
static int dirfree(int id)
{
	int index = id / 64;
	int shift = 63 - id % 64;
	
	printf("dirfree index %d shift %d\n",index, shift);
	
	unsigned long long test_b = 1LL << shift;
	
	printf("dirfree %llx test_b %llx\n", g_dir_info.dmap[index], test_b);
	
	g_dir_info.dmap[index] = g_dir_info.dmap[index] ^ test_b;
	
	
	printf("dirfree %llx\n", g_dir_info.dmap[index]);
	//目录个数减一
	--g_dir_info.size;
	
	return 0;	
}

/*
 * 分配i节点
 */
static int ialloc()
{
	int index = IMAP_SIZE;
	 
	 
	 while(index >= 0 && g_imap.bits[--index] == 0);
	 
	 //没有空闲的空间
	 if(index < 0)
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
	int shift = 63 - id % 64;
	
	unsigned long long test_b = 1LL << shift;
	g_imap.bits[index] = g_imap.bits[index] ^ test_b;
	
	++g_sn.s_ninode;
	
	return 0;
	
}

/*
 * 分配物理块
 */
static int balloc()
{
	int index = BMAP_SIZE;
	 
	 
	 while(index >= 0  && g_bmap.bits[--index] == 0);
	 
	 //没有空闲的空间
	 if(index < 0)
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
	int shift = 63 - id % 64;
	
	unsigned long long test_b = 1LL << shift;
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

	printf("\tls\t\t显示当前目录的内容。\n\tmkdir\t\t创建目录。\n\trmdir\t\t删除目录。\n\tchdir or cd\t更改当前工作目录。\n\tcreate or crt\t创建文件。\n\tdelete or del or rm\t删除文件。\n\topen\t\t打开文件。\n\tclose\t\t关闭文件。\n\twrite or wr\t向文件中写数据。\n\tread or rd\t读文件中的数据。\n\tquit\t\t退出。\n\tlogin\t\t用户登录。\n\tlogout\t\t用户登出。\n\tpwd\t\t显示当前工作目录。\n\tcat\t\t显示文件内容。\n");
	return ;
}
