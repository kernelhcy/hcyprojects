#include "user.c"

/*
 * 查找用户是否存在。
 * 如存在，则返回0并填充用户信息到参数usr中，否则返回-1.
 */
int find_usr(const char *username, struct user *usr)
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
int create_user(const char *username, const char *passwd)
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
	hfs_login(username, passwd);
}

/*
 * 访问控制函数。
 * 判断当前用户是否对文件和目录用权限进行相应的操作。
 *
 * 返回值： 
 * 	0: 		有权限。
 * 	其他值:	没有权限。
 */
static int access_f(const char* name, int mode)
{
	if(name == NULL)
	{
		return -1;	
	}
	
	int i_id = find(name);
	if(i_id < 0)
	{	
		printf("文件不存在！！\n");
		return -1;
	}

	//所有者的权限
	int own_r = dinodes[i_id].di_right/10;
	//其他用户的权限
	int othr_r = dinodes[i_id].di_right%10;

	if(dinodes[i_id].di_uid == login_users[curr_usr_id].p_uid)
	{
		return (mode & own_r);
	}
	else
	{
		return (mode & othr_r);
	}
}
