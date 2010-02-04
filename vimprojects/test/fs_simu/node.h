#ifndef __HFS_NODE_H
#define __HFS_NODE_H
/**
 * 定义i节点，磁盘i节点，以及物理块的实现和操作。
 * by hcy
 */
#include "global_data.h"
#define BLOCK_SIZE 16	  		//物理块的大小
#define B_ADDR_NUM BLOCK_SIZE/4	//每个物理块所能存放的物理块地址的个数
#define D_ADDR_NUM 5     		//每个文件直接索引块的个数

#define INODE_MAXNUM 1024    	//i节点的最大个数
#define BNODE_MAXNUM 65536 	 	//物理节点的最大个数

/*
 * 内存i节点
 */
struct inode
{
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
	//存储的是物理块的下标
	unsigned int direct_addr[D_ADDR_NUM];  	
	unsigned int addr;						/*一级块索引*/
	unsigned int sen_addr;					/*二级块索引*/
	unsigned int tru_addr;					/*三级块索引*/
};


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
 * 获得和释放一个i节点。
 * 获得i节点将返回一个空闲的i节点的下标。分配失败将返回-1
 * 释放i节点，将其标记为空闲
 */
int ialloc();
void ifree(int);

/*
 * 获取和释放物理块
 */
int balloc();
void bfree(int);

#endif
