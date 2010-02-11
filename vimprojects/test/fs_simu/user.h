#ifndef __HFS_USER_H
#define __HFS_USER_H

#include "fs_base.h"

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
