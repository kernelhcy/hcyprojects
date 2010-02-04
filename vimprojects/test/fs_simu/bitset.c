#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bitset.h"


//计算一个size_t类型有占多少位。
//CHAR_BIT表示一个char类型占多少为，在/usr/include/limits.h中定义，本人机器中定义为8.
#define BITSET_BITS \
	( CHAR_BIT * sizeof(unsigned long long int) )

/**
 * 得到一个pos位置为1,其他位置都为0的size_t类型的掩码。
 * 其中pos位置是这个位在bitset中的位置，因此要模一个BITSET_BITS才是其在size_t中的位置。
 */
#define BITSET_MASK(pos) \
	( (1ULL) << ((pos) % BITSET_BITS) )
/**
 * 计算pos位在set中的bits数组中的位置。
 * 也就是，pos位在数组bits中，包含在那个size_t类型的成员中。
 */
#define BITSET_WORD(set, pos) \
	( (set)->bits[(pos) / BITSET_BITS] )

/**
 * 由于bitset中是用size_t类型数组来存放bit位的，因此实际开的空间应该是size_t的整数倍。
 * 这个宏就是用来计算在需要nbits个位的情况下，要开多少内存空间。
 * 也就是计算nbits是BITSET_BITS的整数倍加一。
 */
#define BITSET_USED(nbits) \
	((size_t)( ((nbits) + (BITSET_BITS - 1)) / BITSET_BITS ))


/**
 * 初始化一个bitset为nbits位
 */
bitset *bitset_init(size_t nbits)
{
	bitset *set;

	set = malloc(sizeof(*set));
	if (!set)
	{
		return NULL;
	}

	//分配空间并初始化为0.
	set -> bits = calloc(BITSET_USED(nbits), sizeof(*set->bits));
	set -> nbits = nbits;
	set -> array_len = BITSET_USED(nbits);

	return set;
}

/**
 * 将set中的所有位重置为0
 */
void bitset_reset(bitset * set)
{
	memset(set->bits, 0, BITSET_USED(set->nbits) * sizeof(*set->bits));
}

//释放set
void bitset_free(bitset * set)
{
	free(set->bits);
	free(set);
}

//将pos位设置为0.
void bitset_clear_bit(bitset * set, size_t pos)
{
	if (pos >= set->nbits)
	{
		log_error("bitset test error! %s %d", __FILE__, __LINE__);
	}

	BITSET_WORD(set, pos) &= ~BITSET_MASK(pos);
}
//将pos为设置为1
void bitset_set_bit(bitset * set, size_t pos)
{
	if (pos >= set->nbits)
	{
		log_error("bitset test error! %s %d", __FILE__, __LINE__);
	}

	BITSET_WORD(set, pos) |= BITSET_MASK(pos);
}
//测试pos位置是否是1
int bitset_test_bit(bitset * set, size_t pos)
{
	if (pos >= set->nbits)
	{
		log_error("bitset test error! %s %d", __FILE__, __LINE__);
	}

	return (BITSET_WORD(set, pos) & BITSET_MASK(pos)) != 0;
}

size_t bitset_get_first_unused_bit_pos(bitset *set)
{
	if (set == NULL)
	{
		return ERROR_VAL;
	}

	size_t pos;

	size_t index = 0;
	/*
	 * 如果没有空位，则bits数组中的元素应该是unsigned long long int 
	 * 类型得最大值 ~((unsigned long long int)0).
	 * 所以，判断这个位置是否是这个最大值就可以知道是否在这个位置有
	 * 空的bit。
	 */
	while (index < BITSET_USED(set -> nbits)
			&& set -> bits[++index] == ~((unsigned long long int)0));

	if (index >= BITSET_USED(set -> nbits))
	{
		log_error("bitset has no unused bit.\n");
		return ERROR_VAL;
	}
	--index;
	pos = index * BITSET_BITS;
	while (pos <= set -> nbits && bitset_test_bit(set, pos++));
	--pos;
	return pos;
}

void bitset_print(bitset* set)
{
	if (NULL == set)
	{
		return;
	}
	
	printf("Print BitSet:");
	
	int i, line_no = BITSET_BITS / 2;
	for (i = 0; i < set -> nbits; ++i)
	{
		if (i % line_no == 0)
		{
			printf("\n");
		}
		printf("%d ", bitset_test_bit(set, i));
	}
	printf("\n");
	printf("Print BitSet End.\n");

	return;
}

int bitset_write_fd(bitset* set, FILE* fd)
{
	if (NULL == set || NULL == fd)
	{
		return 0;
	}
	int len = 0;

	len += fwrite(set -> bits, sizeof(*(set -> bits)) , set -> array_len, fd);
	len += fwrite(&set -> nbits, sizeof(set -> nbits), 1, fd);
	len += fwrite(&set -> array_len, sizeof(set -> array_len), 1, fd);

	return len;
}
int bitset_read_fd(bitset* set, FILE* fd)
{
	if (NULL == set || NULL == fd)
	{
		return 0;
	}

	int len = 0;

	len += fread(set -> bits, sizeof(*(set -> bits)) , set -> array_len, fd);
	len += fread(&set -> nbits, sizeof(set -> nbits), 1, fd);
	len += fread(&set -> array_len, sizeof(set -> array_len), 1, fd);
	
	return len;
}

