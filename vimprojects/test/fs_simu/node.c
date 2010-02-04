#include "node.h"

/*
 * 查找是否有可用的i节点
 * 如果有，则返回其下标位置;如果没有，返回-1;
 * 查找是对i节点位图进行。
 */
int ialloc()
{
	size_t pos = bitset_get_first_unused_bit_pos(g_imap);
	if (pos == ERROR_VAL)
	{
		return -1;
	}
	return (int)pos;
}
/*
 * 释放i节点，标记位图上未用。
 */
void ifree(int id)
{
	if (id < 0)
	{
		return;
	}
	bitset_set_bit(g_imap, (size_t)id);
}

/*
 * 获得物理块
 */
int balloc()
{
	size_t pos = bitset_get_first_unused_bit_pos(g_bmap);
	if (pos == ERROR_VAL)
	{
		return -1;
	}
	return (int)pos;
}
//释放物理块
void bfree(int id)
{
	if (id < 0)
	{
		return;
	}
	bitset_set_bit(g_bmap, (size_t)id);
}
