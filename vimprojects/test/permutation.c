#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * 按照字典序，输出排列p的下一个排列。
 * 输出的结果保存在next中。
 * len为排列长度。
 *
 * 返回0表示没有后一个了。
 *
 */
int permutation(int *p, int *next, int len)
{
	if (NULL == p || NULL == next || len == 0)
	{
		return 0;
	}

	int ndx = len - 1;
	while (ndx > 0)
	{
		if (p[ndx - 1] < p[ndx])
		{
			--ndx;
			break;
		}
		--ndx;
	}

	if (ndx <= 0 && p[0] >= p[1])
	{
		return 0;
	}

	int ndx2 = len - 1;
	while (p[ndx] >  p[ndx2])
	{
		--ndx2;
	}

	memcpy(next, p, sizeof(int) * len);
	
	int tmp;
	tmp = next[ndx];
	next[ndx] = next[ndx2];
	next[ndx2] = tmp;
	
	ndx2 = len - 1;
	for(++ndx;ndx <= ndx2; ++ndx, --ndx2)
	{
		tmp = next[ndx];
		next[ndx] = next[ndx2];
		next[ndx2] = tmp;
	}

	return 1;
}

void print(int *p, int len)
{
	int i;
	for (i = 0; i < len; ++i)
	{
		printf("%d ", p[i]);
	}
	printf("\n");
}


int main(int argc, char *argv[])
{
	int p[] = {1,2,3,4,5};
	int len = 5;
	
	print(p, len);
	while(permutation(p, p, len))
	{
		print(p, len);
	}
	
/*
	permutation(p, p, len);
	print(p, len);
	permutation(p, p, len);
	print(p, len);
	permutation(p, p, len);
	print(p, len);
	permutation(p, p, len);
	print(p, len);
	permutation(p, p, len);
	print(p, len);
*/	
	return 0;
}

