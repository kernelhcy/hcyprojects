package lis;

public class LIS
{
	private LIS() {
		System.out.println("Can not creat the instance of LIS!");
	}

	public static int getLISLen(final char[] s, char[] lis)
	{
		if (null == s) {
			return -1;
		}
		c = new char[s.length + 1];
		c[0] = ' ';
       		c[1] = s[0];
       		len = 1;	//此时只有c[1]求出来，最长递增子序列的长度为1.
		int j;
		for(int i = 0; i < s.length; ++i){
			j = binarySearch(c, len, s[i]);
			c[j] = s[i];
			if(len < j){
				len = j;
			}
			
		}
		System.arraycopy(c, 1, lis, 0, len);
		return len;
	}
	/**
	 * 二分查找。如果返回x，表示a[x] >= n > a[x - 1]
	 * @param a  数组a
	 * @param len 数组a中数据的个数
	 * @param n  需要查找的字符
	 * @return
	 */
	private static int binarySearch(final char[] a, int len, char n)
	{
		if (n < 0) {
			return -1;
		}

		int left = 0, right = len;
		int mid = (left + right) / 2;

		while (left <= right) {
			if (n > a[mid])
				left = mid + 1;
			else if (n < a[mid])
				right = mid - 1;
			else
				return mid;

			mid = (left + right) / 2;
		}
		return left;
	}

	private static int len = 0;
	/*
	 * c[i]=a[j]，表示c[i]中存储的是长度为i的最长递增子列的最后一个字符。
	 * 并且，c中存放的就是最长递增子列。
	 */
	private static char[] c;
}
