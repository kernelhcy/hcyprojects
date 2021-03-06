#ifndef __HFS_FS_H
#define __HFS_FS_H

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "fs_base.h"

/*
 * 定义文件系统支持的基本操作。
 * 并提供有常用的一些其他文件目录操作。
 *
 */

/*
 * 初始化文件系统。
 */
hfs* hfs_init();

/*
 * 退出文件系统。
 * 将数据写回文件。
 */
int hfs_halt(hfs *);

/*
 * 显示帮助信息
 */
void hfs_show_help_info(hfs*);
/*
 * 显示当前目录的内容
 * 参数path为目录路径。
 *
 * 返回目录内容，并以空格分开。
 * 执行失败返回NULL。
 *
 */
char * hfs_ls(hfs *, const char *path);

/*
 * 创建目录
 * 参数path为目录名
 * 创建成功返回0,否则返回错误码。
 */
int hfs_mkdir(hfs *, const char *name);

/*
 * 删除目录
 * 警告：将删除目录中的所有内容！！包括子目录和文件！
 * path：删除的目录的完整路径。
 * 
 * 删除成功返回0,否则返回其他错误码。
 *
 * 只有对目录具有运行权限（X_R），才可以删除目录！！
 *
 */
int hfs_rmdir(hfs *, const char *name);

/*
 * 更换当前工作目录。
 * path：更改到的目录路径。
 *
 * 更改成功放回0,否则返回其他错误码。
 *
 */
int hfs_chdir(hfs *, const char *path);

/*
 * 创建文件。
 * name：文件名。在当前目录中创建文件，只需要文件名。在其他目录中创建文件，需要完整的路径。
 * right：设置文件的权限。由两位十进制数表示。每个数小于8.
 * 		
 * 		文件权限说明：
 * 			每个文件的权限分为两部分：拥有者权限和其他用户权限。
 * 			权限的每个部分又分为三个部分：读，写，运行权限，分别用r,w,x表示。
 *          如：rwxr--表示拥有者对此文件具有读写和运行的权限，其他用户仅有读的权限。
 *
 *          对权限表示进行变换。具有权限用1表示，不具有的权限用0表示。
 *          则可用一个六位的二进制数表示一个
 *          文件的权限。如：rwxr--表示为111100。对这个二进制数没三位分一组，
 *          并将组中的三位二进制数表示
 *          成十进制数。上例可表示为71.
 *
 *          如：rw-rw-为66，
 *             rwxrw-为76，
 *             r--r--为44。
 *
 * 创建成功返回文件指针，否则返回NULL。
 */
FILE_P* hfs_create(hfs *, const char *name, int right);

/*
 * 删除文件。
 * name：完整的路径和文件名。
 *
 * 删除成功返回0,否则返回其他错误码。
 *
 */
int hfs_delete(hfs *, const char *name);

/*
 * 打开文件。
 * name：完整的路径和文件名。
 * mode：打开方式。
 *
 * 		打开方式说明：
 * 			有三种打开方式：R（读），W（写），A（追加）。
 * 				R（1,读）：以只读的方式打开文件。不能对文件进行修改操作。
 * 				W（2,写）：以只写的方式打开文件。文件不存在时，将创建文件。若文件已经存在，
 * 						 则文件中原先的内容将被覆盖！
 * 				A（4,追加）：以追加的方式打开文件。文件不存在时报错！！数据追加到文件的尾部，
 * 				         对以前的内容没有影响。
 * 		    
 * 		    	文件的打开方式可以通过或的方式组合使用。如：R|W表示一读写的方式打开，R|A表示一读追加的方式打开。
 * 		    	注：
 * 		    		三种模式都使用则忽略第三个！
 * 		    		如：R|W|A则相当于R|W。R|A|W相当于R|A。
 * 
 * 返回值：
 * 		文件指针。
 * 		返回NULL，则打开失败。
 */
FILE_P* hfs_open(hfs *,const char *name, int mode);

/*
 * 关闭文件。
 * fp：文件指针。
 *
 * 返回值：
 * 		0：成功。
 * 		其他值失败。
 */
int hfs_close(hfs *, FILE_P* fp);
/*
 * 向文件中写数据。
 * fp：文件指针。
 * buffer：数据缓冲区。
 * length：缓冲区buffer中需要写入文件的数据长度。
 *
 * 返回值：
 * 		-1：写入失败。
 * 		写入成功则返回写入的数据长度。
 */
int hfs_write(hfs *, FILE_P* fp, char *buffer, int length);
/*
 * 读文件中的数据。
 * fp：文件指针。
 * buffer：缓冲区。存放读入的数据。
 * length：要读入的数据的长度。
 *
 * 返回值：
 * 		-1：读取失败。
 * 		否则返回读取的数据的长度。
 */
int hfs_read(hfs *, FILE_P* fp, char *buffer, int length);
/*
 * 显示当前工作目录。
 * 
 * 返回值：
 * 		当前目录路径
 */
char* hfs_pwd(hfs *);
/*
 * 登录或创建用户。
 *
 * user_name：用户名。
 * passwd：密码。
 *
 * 当用户名不存在时，提示是否创建用户。
 *
 * 登录或创建成功，返回0,否则返回其他错误码。
 *
 */
int hfs_login(hfs *, const char *username, const char *passwd);
/*
 * 用户登出。
 *
 * user_name：用户名。
 *
 * 退出成功，放回0,否则返回其他错误码。
 */
int hfs_logout(hfs *, const char *username);
/*
 * 显示文件内容。
 * 
 * fp：文件指针。
 *
 * 返回文件的内容。
 *
 */
char * hfs_cat(hfs *, FILE_P* fp);

/*
 * 以命令行的形式运行文件系统
 */
void run(hfs *, int showdetails);

#endif
