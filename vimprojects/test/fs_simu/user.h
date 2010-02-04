#ifndef __HFS_USER_H
#define __HFS_USER_H

#include "global_data.h"

/**
 * 定义文件系统中用户的操作和常量
 *
 */
#define USR_NAME_MAXSIZE 64    	//用户名的长度
#define PWD_MAXSIZE 20     		//用户密码长度
#define USR_OFILE_MAXNUM 100   	//允许用户打开的文件最大个数

#define MAX_LOGIN_USR 100 		//允许同时登录的最大用户个数

/*
 * 用户
 */
struct user
{
 	unsigned short p_uid;
	unsigned short p_gid;
	char username[USR_NAME_MAXSIZE];
	char passwd[PWD_MAXSIZE];
};

/*
 * 用户打开表
 */
struct user_ofile
{ 
	unsigned short u_default_mode;
	unsigned short u_uid; 			/*用户标志*/
	unsigned short u_gid; 			/*用户组标志*/
	unsigned short u_ofile[USR_OFILE_MAXNUM]; /*用户打开表*/
};

/*
 * 查找用户是否存在。
 * 如存在，则返回0并填充用户信息到参数usr中，否则返回-1.
 */
int find_usr(const char *username, struct user *usr);

/*
 * 创建用户。
 */
int create_user(const char *username, const char *passwd);

int access_f(const char * name, int mode);
#endif
