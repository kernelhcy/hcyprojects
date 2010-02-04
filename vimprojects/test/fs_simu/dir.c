#include "dir.h"

/*
 * 解析目录路径，查找文件和目录。
 * 获得其inode号。 
 * 当路径有错误时，返回-1，否则返回inode号。
 */
int find(const char * path);
{
	char tmp_name[DIR_NAME_MAXSIZE];	//当前正在解析的文件名
	int i_id = 0;					//当前正在解析的文件inode号
	int tmp_dir_id = 0;             //若当前解析的名称是目录的名称，记录其目录号。	
	int result = -1;				//最终的结果

	int index = 0;
	int begin,end;
	
	
	
	while(path[index] != '\0' && path[index] != '/')
	{
		++index;
	}
	
	while(path[index] != '\0')
	{

		++index;
	
		//获得文件名的开始和结束位置。
		begin = index ;
		while(path[index] != '\0' && path[index] != '/')
		{
			++index;
		}
		end = index;
		
		memset(tmp_name,'\0',DIR_NAME_MAXSIZE);
		str_cpy(tmp_name, path, begin, end - begin);
		//printf("pares: %s\n",tmp_name);	
		int tmp = -1;
		
		//查看子目录中是否有此文件。
		//从根目录开始搜索。
		int i;
		for(i = 0; i < dir_table[tmp_dir_id].sub_cnt; ++i)
		{
			//printf("%s\n",dir_table[tmp_dir_id].file_name[i]);
			if(strcmp(tmp_name, dir_table[tmp_dir_id].file_name[i]) == 0)
			{
				tmp = i;
				break;
			}
		}
		
		if(tmp < 0)//路径有错误。
		{
			return -1;
		}
		
		if(dir_table[tmp_dir_id].file_inode[tmp] != 0)
		{
			i_id = dir_table[tmp_dir_id].file_inode[tmp];
			tmp_dir_id = dir_table[tmp_dir_id].sub_dir_ids[tmp];
		}
		result = i_id;
	}
	
	return result;
}



/*
 * 分配目录表
 * 返回分配的目录表项的索引。
 */
int diralloc()
{
	bitset_print(g_dir_info -> dmap);
	size_t pos = bitset_get_first_unused_bit_pos(g_dir_info -> dmap);
	if (pos == ERROR_VAL)
	{
		return -1;
	}
	bitset_set_bit(g_dir_info -> dmap, pos);
	return (int)pos;
}

/*
 * 释放目录项
 */
int dirfree(int id)
{
	bitset_clear_bit(g_dir_info -> dmap, (size_t)id);
	--g_dir_info -> size;
	
	return 0;	
}
