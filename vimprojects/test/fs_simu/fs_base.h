#ifndef _FS_STRUCTS_H
#define _FS_STRUCTS_H
#include "bitset.h"
#include "node.h"
/*
 * 定义文件系统的基本数据结构常量等。
 */

/*
 * 文件目录标记。
 * 用于在i节点中标记此节点表示的是文件还是目录。
 */
#define FILE_T 111				//表示文件
#define DIR_T  222 				//表示目录
#define NONE_T 333 				//未定义类型
//文件的三种打开方式
#define R 1						//只读方式
#define W 2						//写方式
#define A 4						//追加方式
//用于对文件可以拥有的三种权限
#define R_R 1					//读权限
#define W_R 2					//写权限
#define X_R 4					//运行权限

#define DIR_NAME_MAXSIZE 128   	//目录名的最大长度
#define FILE_NAME_MAXSIZE 128	//文件名的最大长度
#define DIR_MAXNUM 2048	     	//文件系统支持目录总的最大个数
#define DIR_INCLUDE_MAXNUM 128  //每个目录中能包含的最大的文件或目录个数

//文件指针
typedef struct inode FILE_P;

/*
 * 超级块
 */
struct supernode
{
 	unsigned int s_isize;            	/*i节点块块数*/
 	unsigned long s_bsize;             	/*数据块块数*/

	

 	unsigned int s_nfree;             	/*空闲块块数*/
 	unsigned short s_pfree;           	/*空闲块指针*/
 	unsigned int s_free[NICFREE];     	/*空闲块堆栈*/


 	unsigned int s_ninode;            	/*空闲i节点数*/
 	unsigned short s_pinode;          	/*空闲i节点指针*/
 	unsigned int s_inode[NICINOD];   	/*空闲i节点数组*/
	unsigned int s_rinode;           	/*铭记i节点*/
 	char s_fmod;                    	/*超级块修改标志*/
 	
 	long disk_size;						/*磁盘空间大小。用文件大小模拟。*/
};

/*
 * 系统打开表
 */
struct system_ofile
{
	char f_flag; 			/*文件操作标志*/
	unsigned int f_count; 	/*引用计数*/
	struct inode *f_inode; 	/*指向内存节点*/
	unsigned long f_off; 	/*读/写指针*/
	
	
};

#endif
