#ifndef _FS_STRUCTS_H
#define _FS_STRUCTS_H

/*
 * 定义文件系统所用到的数据结构及常量
 * 各个常量定义的过大会导致段错误！！
 */

/*
 * 文件目录标记。
 * 用于在i节点中标记此节点表示的是文件还是目录。
 */
#define FILE_T 111				//表示文件
#define DIR_T  222 				//表示目录
#define R 1						//只读方式
#define W 2						//写方式
#define A 4						//追加方式

#define R_R 1						//读权限
#define W_R 2						//写全信
#define X_R 4						//运行权限

#define BLOCK_SIZE 16	  		//物理块的大小
#define B_ADDR_NUM BLOCK_SIZE/4	//每个物理块所能存放的物理块地址的个数
#define D_ADDR_NUM 5     		//每个文件直接索引块的个数

#define DIR_NAME_SIZE 128   	//目录名的最大长度
#define FILE_NAME_SIZE 128		//文件名的最大长度
#define DIR_NUM 2048	     	//文件系统支持目录总的最大个数
#define DMAP_SIZE DIR_NUM/64	//目录表位图的长度
#define DIR_INCLUDE_NUM 128	    //每个目录中能包含的最大的文件或目录个数

#define USR_NAME_SIZE 64    	//用户名的长度
#define PWD_SIZE 20     		//用户密码长度
#define USR_OFILE_NUM 100   	//允许用户打开的文件最大个数

#define INODE_SIZE 1024    		//i节点的最大个数
#define BNODE_SIZE 65536 		//物理节点的最大个数

#define NICFREE BNODE_SIZE     	//空闲物理块栈的大小
#define NICINOD INODE_SIZE     	//空闲i节点数组大小

#define IMAP_SIZE INODE_SIZE/64	//i节点位图的长度
#define BMAP_SIZE BNODE_SIZE/64 //物理块位图的长度

#define MAX_LOGIN_USR 100 		//允许同时登录的最大用户个数


/*
 * 内存i节点
 */
typedef struct inode{
	struct inode  *i_forw;
	struct inode  *i_back;
	
	int dir_or_file;					/*标记是文件还是目录*/
	
	unsigned int i_into;      			/*磁盘i节点标号*/
	unsigned int i_count;     			/*用户引用计数*/
	unsigned short di_number; 			/*关联文件数，当为0时，则删除该文件*/
	unsigned int di_right;   			/*存取权限*/
	unsigned int mode;					/*打开方式*/
	
	unsigned short di_uid;    			/*磁盘i节点用户*/
	unsigned short di_gid;    			/*磁盘i节点组*/
	unsigned int parent_id;             /*父目录的目录号。*/

	unsigned long di_size;            	/*文件大小*/
	unsigned long curr_pos;				/*文件读取的当前位置。*/
	/*
	 * 直接物理块索引。
	 *
	 *	每个物理块可以存放的地址个数为8.
	 *  直接索引为5个。一级索引为8个，二级为64个，三级为512个。总索引为589个。
	 *  每个物理块的大小为32byte。
	 *  文件的最大长度为18848byte=18.4kb。
	 *
	 * 		若i节点为目录的i节点，
	 *       直接索引的第一个存放其目录表中的位置！下标。
	 */
	unsigned int direct_addr[D_ADDR_NUM];  	
	unsigned int addr;						/*一级块索引*/
	unsigned int sen_addr;					/*二级块索引*/
	unsigned int tru_addr;					/*三级块索引*/
}FILE_P;


/*
 * 磁盘i节点
 */
struct dinode
{
	int dir_or_file;					/*标记是文件还是目录*/
 	unsigned short di_number;        	/*关联文件数*/
 	unsigned int di_right;          	/*存取权限*/

 	unsigned int di_uid;				/*所有者的id*/
 	unsigned int di_gid;
	
	unsigned int parent_id;             /*父目录的目录号。*/

	unsigned long di_size;            	/*文件大小*/
	
	/*
	 * 直接物理块索引。
	 *
	 *	每个物理块可以存放的地址个数为8.
	 *  直接索引为5个。一级索引为8个，二级为64个，三级为512个。总索引为589个。
	 *  每个物理块的大小为32byte。
	 *  文件的最大长度为18848byte=18.4kb。
	 *  
	 * 		若i节点为目录的i节点，
	 *     直接索引的第一个存放其目录表中的位置！下标。
	 */
	unsigned int direct_addr[D_ADDR_NUM];  	
	
	unsigned int addr;						/*一级块索引*/
	unsigned int sen_addr;					/*二级块索引*/
	unsigned int tru_addr;					/*三级块索引*/
};

/*
 * 模拟物理块。
 * 使用联合体，同时可以存放数据和物理块的地址。方便操作。
 */
union block
{
	char entry[BLOCK_SIZE];
	unsigned int b_addr[B_ADDR_NUM]; 
};

/*
 * i节点位图
 */
struct imap
{
	unsigned long long bits[IMAP_SIZE];
};

/*
 * 物理块位图
 */
struct bmap
{
	unsigned long long bits[BMAP_SIZE];
};

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
 * 用户
 */
struct user
{
 	unsigned short p_uid;
	unsigned short p_gid;
	char username[USR_NAME_SIZE];
	char passwd[PWD_SIZE];
};

/*
 * 单个目录结构
 */
struct directory
{
 	char d_name[DIR_NAME_SIZE];         		/*目录名*/
 	unsigned int d_id;							/*目录的id号。即目录数组中的索引号*/
 	unsigned int inode_id;						/*inode号*/
 	unsigned int parent_id;						/*父目录的id。*/
 	
 	unsigned int sub_cnt;									/*子目录和文件的数目*/
	char file_name[DIR_INCLUDE_NUM][FILE_NAME_SIZE];        /*目录中的文件或子目录的名子*/
	unsigned int file_inode[DIR_INCLUDE_NUM];           	/*对应的inode号*/
	unsigned int sub_dir_ids[DIR_INCLUDE_NUM];				/*若对应项是目录，存放其id。*/
	
};

/*
 * 目录信息
 */
struct dir_info
{
 	unsigned int root_id;						//根目录的id。通常是0.
 	unsigned int size;                  		//目录的个数
	/*
	 * 用于记录当前可用的位置。
	 */
	int index;                                  //目录表中可用的位置。
 	unsigned long long dmap[DMAP_SIZE];	//位图
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
