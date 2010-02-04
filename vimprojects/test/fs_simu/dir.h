#ifndef __HFS_DIR_H
#define __HFS_DIR_H
/**
 * 定义关于目录的操作和数据结构
 *
 */

/*
 * 单个目录结构
 */
struct directory
{
 	char d_name[DIR_NAME_MAXSIZE];         		/*目录名*/
 	unsigned int d_id;							/*目录的id号。即目录数组中的索引号*/
 	unsigned int inode_id;						/*inode号*/
 	unsigned int parent_id;						/*父目录的id。*/
 	
 	unsigned int sub_cnt;									/*子目录和文件的数目*/
	char file_name[DIR_INCLUDE_MAXNUM][FILE_NAME_MAXSIZE];        /*目录中的文件或子目录的名子*/
	unsigned int file_inode[DIR_INCLUDE_MAXNUM];           	/*对应的inode号*/
	unsigned int sub_dir_ids[DIR_INCLUDE_MAXNUM];				/*若对应项是目录，存放其id。*/
	
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
	bitset *dmap; 								//位图
};

/*
 * 解析目录路径，查找文件和目录。
 * 获得其inode号。 
 * 当路径有错误时，返回-1，否则返回inode号。
 */
int find(const char * path);

/*
 * 分配目录表
 * 返回分配的目录表项的索引。
 */
int diralloc();

/*
 * 释放目录项
 */
int dirfree(int id);

#endif


