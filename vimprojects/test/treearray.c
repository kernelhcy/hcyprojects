#include <stdlib.h>
#include <stdio.h>

/**
 *
 * 一维树状数组。
 *
 * 树状数组的每个元素并不像普通数组那样只保存自己的值，而是保存了从它开始前2^k个值的和
 * (k为当前位置的下标的二进制的最后面的0的个数）
 * 如：
 * -----------------------------------------------------------------------
 * | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16|
 * -----------------------------------------------------------------------
 * 上面表示一个数组，数字是下标。(数组的第一个位置不使用)
 * 显然，对于奇数，二进制表示中最后一位必然是0,因此，奇数位置都保存自己的值。
 * 对于偶数的位置，其二进制表示的末尾零的个数的2次方就相当与将其二进制表示中，从最后一个1开始，
 * 前面所有的1都清零。因此，计算需要O(1)。
 *
 * 如，2后面只有一个0,因此，位置2存放的是1和2的值的和。同样，6存的是5和6位置的和。
 * 	   4后面有两个0.因此存放的是1,2,3和4位置的值的和。
 * 	   8有四个0,存放的是1到8的位置的值的和。
 *
 * 如果要求i->j的和，那么只需要求出1->i-1和1->j的和即可。
 */


#define MAX 1025

int a[MAX]; 	//a[0]不使用。 

//求x的二进制表示尾部0的个数的2次方。
int lowbit(int x)
{
	return x & (x ^ (x - 1));
}

/**
 * 求从1到n的和
 */
int sum(int n)
{
	int re = 0;
	while(n > 0)
	{
		re += a[n];
		n -= lowbit(n);
	}

	return re;
}

/**
 * 将数组中，位置为i的值修改为val。
 */
void change(int i, int val)
{
	while (i <= MAX)
	{
		a[i] += val;
		i += lowbit(i);
	}
}
	
/**
 * 二维树状数组。
 * aa[i][j]存放的是：
 * 	i的二进制表示的末尾零的个数的2次方为m，
 * 	j的二进制表示的末尾零的个数的2次方为n，
 * 	那么，其值为从aa[i - m + 1][i - j + 1]到aa[i][j]构成的一个矩形中的数据的和。
 */
long long aa[MAX][MAX]; 
long long s;
long long sum2(long long i,long long j)
{   
    long long sum;
    while(i > 0)
    {  
       	long long tmp = j;
        while(tmp > 0)
        {   
          	sum += aa[i][tmp];
            tmp -= lowbit(tmp);
        }
        i -= lowbit(i);
    }
   	return sum;
}
void change2(long long i,long long j,long long k)
{   
   	while(i <= s)
    {  
       	long long tmp = j;
        while(tmp <= s)
        {   
          	aa[i][tmp] += k;
            tmp += lowbit(tmp);
       	}
       	i += lowbit(i);
 	}
}	

void clear(long long s)
{
	long long i, j;
	for (i = 0; i <= s; ++i)
	{
		for (j = 0; j <=s; ++j)
		{
			aa[i][j] = 0;
		}	
	}
}

long long range_sum(long long x, long long y, long long x1, long long y1)
{
	long long s1 = sum2(x - 1, y - 1);
	long long s2 = sum2(x1, y1);
	long long s3 = sum2(x1, y - 1);
	long long s4 = sum2(x - 1, y1);

	return s2 - s3 - s4 + s1;
}


int main(int argc, char *argv[])
{
	freopen("in", "r", stdin);
	long long ins;
	
	long long x, y, a;
	long long l, b, r, t;
	while (1)
	{
		scanf("%d", &ins);
		if (3 == ins)
		{
			break;
		}

		switch(ins)
		{
			case 0:
				scanf("%d", &s);
				clear(s);
				break;
			case 1:
				scanf("%d%d%d", &x, &y, &a);
				change2(x + 1, y + 1, a);
				break;
			case 2:
				scanf("%d%d%d%d", &l, &b, &r, &t);
				printf("%lld\n", range_sum(l + 1, b + 1, r + 1, t + 1));
				break;
			default:
				break;
		}
	}

	return 0;
}
