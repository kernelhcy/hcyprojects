#ifndef _FS_H
#define _FS_H
/*
 * 显示帮助信息
 */
void show_help_info();
/*
 * 显示当前目录的内容
 */
int ls();
/*
 * 创建目录
 */
int mkdir();
/*
 * 删除目录
 * 警告：将删除目录中的所有内容！！包括子目录和文件！
 */
int rmdir();
/*
 * 更换当前工作目录。
 */
int chdir();
/*
 * 创建文件。
 */
int create_f();
/*
 * 删除文件。
 */
int delete_f();
/*
 * 打开文件。
 */
int open_f();
/*
 * 关闭文件。
 */
int close_f();
/*
 * 向文件中写数据。
 */
int write_f();
/*
 * 读文件中的数据。
 */
int read_f();
/*
 * 显示当前工作目录。
 */
int pwd();
/*
 * 登录或创建用户。
 */
int login();
/*
 * 用户登出。
 */
int logout();
/*
 * 显示文件内容。
 */
int cat();
#endif
