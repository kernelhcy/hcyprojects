#ifndef __HFS_BITSET_H
#define __HFS_BITSET_H

#include <stddef.h>

#define ERROR_VAL (~((size_t)0))

/**
 * 定义bitset结构
 * bits指向一个size_t类型的数组，存放bit集合
 * nbits记录bitset中bit为的个数。
 * array_len为数组bits的长度
 */
typedef struct 
{
	unsigned long long int * bits;
	size_t nbits;
	size_t array_len;
} bitset;
/**
 *  初始化一个bitset为nbits位。
 */
bitset *bitset_init(size_t nbits);
/**
 * 重置一个bitset
 */
void bitset_reset(bitset * set);
/**
 * 释放set
 */
void bitset_free(bitset * set);
/**
 * 设置pos位为0
 */
void bitset_clear_bit(bitset * set, size_t pos);
/**
 * 设置pos位为1
 */
void bitset_set_bit(bitset * set, size_t pos);
/**
 * 测试pos位是否是1
 */
int bitset_test_bit(bitset * set, size_t pos);
/**
 * 获得第一个未使用的位的下标
 * 如果没有找到，则返回size_t的最大值： ~((size_t)0)
 */
size_t bitset_get_first_unused_bit_pos(bitset *set);

/**
 * 打印位集合
 */
void bitset_print(bitset* set);

/*
 * 向文件中写和从文件中读数据。
 */
int bitset_write_fd(bitset* set, FILE* fd);
int bitset_read_fd(bitset* set, FILE* fd);


#endif
