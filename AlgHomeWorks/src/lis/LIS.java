package lis;

public class LIS
{
	private LIS() {
		System.out.println("Can not creat the instance of LIS!");
	}

	public static int getLISLen(final int[] s, int[] lis)
	{
		if (null == s) {
			return -1;
		}
		c = new int[s.length + 1];
		cindex = new int[s.length + 1];
		pre = new int[s.length];
		
		//初始化
       		cindex[0] = -1;
       		for(int i = 0; i < s.length; ++i){
       			pre[i] = -1;
       			cindex[i + 1] = -1;
       		}
       		
       		c[0] = 0; //这个元素作为一个哨兵。在二分查找中使用。
       		c[1] = s[0];
       		cindex[1] = 0;
       		len = 1;	//此时只有c[1]求出来，最长递增子序列的长度为1.
		int j;
		for(int i = 1; i < s.length; ++i){
			j = binarySearch(c, len, s[i]);
			c[j] = s[i];
			cindex[j] = i;
			/*
			 * 以s[i]结尾的最长子串的倒数第二个字符是c[j-1]。
			 */
			pre[i] = cindex[j - 1];
			if(len < j){
				len = j;
				lastIndex = i;
			}
			
		}
		getSubsquence(s, lis);
		return len;
	}
	/**
	 * 二分查找。返回值表示n在数组a中的位置。如果在数组中有元素等于n
	 * 那么返回最后一个等于n的元素的下一个位置。
	 * @param a  数组a
	 * @param len 数组a中数据的个数
	 * @param n  需要查找的字符
	 * @return
	 */
	private static int binarySearch(final int[] a, int len, int n)
	{
		if (n < 0) {
			return -1;
		}

		int left = 0, right = len;
		int mid = (left + right) / 2;

		while (left <= right) {
			/*
			 * 等于是为了处理"两个相等的元素"也是递增序列的情况
			 */
			if (n >= a[mid]){
				left = mid + 1;
			}
			else if (n < a[mid]){
				right = mid - 1;
			}
			
			mid = (left + right) / 2;
		}
		return left;
	}
	
	/**
	 * 构造其中一个最长递增子列。
	 * @param s 原始字符串。
	 * @param lis 最长子列
	 */
	private static void getSubsquence(final int[] s, int[] lis)
	{
		int pr;
		int index = len;
		pr = lastIndex;
		do{
			lis[--index] = s[pr];
			pr = pre[pr];
		}while(pr != -1);		
	}
	
	//最长递增子列的长度
	private static int len = 0;
	//最长递增子列最后一个字符的位置。
	private static int lastIndex = -1;
	/*
	 * c[i]=a[j]，表示c[i]中存储的是长度为i的最长递增子列的最后一个字符。
	 * 并且，c中存放的就是最长递增子列。
	 * c从1开始，c[0]最为哨兵在二分搜索中使用
	 */
	private static int[] c;
	/*
	 * cindex[i]存储c[i]对应的字符在字符串中的位置。
	 */
	private static int[] cindex;
	/*
	 * pre[i]表示s[i]所在的最长递增子列的前一个字符的位置。
	 * 注，这个最长子列可能不是s的最长子列，只是包含s[i]中所有
	 * 递增子列最长的。
	 */
	private static int[] pre;
}
