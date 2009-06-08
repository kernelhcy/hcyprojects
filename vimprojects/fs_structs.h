#ifndef _FS_STRUCTS_H
#define _FS_STRUCTS_H

/*
 * 定义文件系统所用到的数据结构及常量
 *
 */

#define BLOCK_SIZE 256  	//物理块的大小
#define NADDR 256     		//每个文件最大使用的物理块数

#define DIR_NAME_SIZE 512   //目录名的最大长度
#define DIR_NUM 512     	//目录的最大个数
#define DIR_FILE 512        //目录中最大的文件或目录个数

#define NICFREE 512     	//空闲物理块栈的大小
#define NICINOD 512     	//空闲i节点数组大小

#define USR_NAME_SIZE 20    //用户名的长度
#define PWD_SIZE 20     	//用户密码长度
#define USR_OFILE_NUM 100   //用户打开的文件最大个数

#define INODE_SIZE 256    	//i节点的最大个数
#define BNODE_SIZE 65536 	//物理节点的最大个数

#define IMAP_SIZE 4       	//i节点位图的长度，INODE_SIZE/64
#define BMAP_SIZE 1024    	//物理块位图的长度，BNODE_SIZE/64
/*
 * i节点
 */
struct inode{
	struct inode  *i_forw;
	struct inode  *i_back;
	
	char i_flag;
	unsigned int i_into;      	/*磁盘i节点标号*/
	unsigned int i_count;     	/*引用计数*/
	unsigned short di_number; 	/*关联文件数，当为0时，则删除该文件*/
	unsigned short di_mode;   	/*存取权限*/

	unsigned short di_uid;    	/*磁盘i节点用户*/
	unsigned short di_gid;    	/*磁盘i节点组*/

	unsigned int di_addr[NADDR];     	/*物理块号*/
};

/*
 * 磁盘i节点
 */
struct dinode
{
 	unsigned short di_number;        	/*关联文件数*/
 	unsigned short di_mode;          	/*存取权限*/

 	unsigned short di_uid;
 	unsigned short di_gid;

	unsigned long di_size;            	/*文件大小*/
	unsigned int di_addr[NADDR];      	/*物理块号*/
};

/*
 * 模拟物理块。
 */
struct block
{
	//int b_id;
	char enrty[BLOCK_SIZE];
};

/*
 * i节点位图
 */
struct imap
{
	long long bits[IMAP_SIZE];
};

/*
 * 物理块位图
 */
struct bmap
{
	long long bits[BMAP_SIZE];
};

/*
 * 目录结构
 */
struct directory
{
 	char d_name[DIR_NAME_SIZE];         /*目录名*/
 	unsigned int d_ino;               	/*目录号*/
	char *file_name[DIR_FILE];          /*目录中的文件的文件名*/
	int file_inode[DIR_FILE];           /*目录中文件的inode号*/
};

/*
 * 超级块
 */
struct supernode
{
 	unsigned short s_isize;            	/*i节点块块数*/
 	unsigned long s_bsize;             	/*数据块块数*/

 	unsigned int s_nfree;             	/*空闲块块数*/
 	unsigned short s_pfree;           	/*空闲块指针*/
 	unsigned int s_free[NICFREE];     	/*空闲块堆栈*/


 	unsigned int s_ninode;            	/*空闲i节点数*/
 	unsigned short s_pinode;          	/*空闲i节点指针*/
 	unsigned int s_inode[NICINOD];   	/*空闲i节点数组*/
	unsigned int s_rinode;           	/*铭记i节点*/
 	char s_fmod;                    	/*超级块修改标志*/
};

/*
 * 用户
 */
struct user
{
 	unsigned short p_uid;
	unsigned short p_gid;
	char username[USR_NAME_SIZE];
	char passward[PWD_SIZE];
};

/*
 * 目录
 */
struct dir
{
 	struct directory dirs[DIR_NUM]; //目录
 	int size;                        //目录的个数
};

/*
 * 查找i内存节点的hash表
 */
struct hinode
{
 	struct inode *iforw;
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

/*
 * 用户打开表
 */
struct user_ofile
{ 
	unsigned short u_default_mode;
	unsigned short u_uid; 			/*用户标志*/
	unsigned short u_gid; 			/*用户组标志*/
	unsigned short u_ofile[USR_OFILE_NUM]; /*用户打开表*/
};

#endif
