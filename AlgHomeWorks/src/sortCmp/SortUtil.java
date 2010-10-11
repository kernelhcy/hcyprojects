package sortCmp;

/**
 * 算法实现
 * @author hcy
 *
 */
public class SortUtil
{
	public static void quickSort(int[] array)
	{
		if(array == null){
			return;
		}
		quickSortHelp(array, 0, array.length);
	}
	
	public static void insertionSort(int[] array)
	{
		if(array == null){
			return;
		}
		insertionSortHelp(array, 0, array.length);
	}
	
	public static void mergeSort(int[] array)
	{
		if(array == null){
			return;
		}
		int[] dest = new int[array.length];
		System.arraycopy(array, 0, dest, 0, array.length);
		mergeSortHelp(dest, array, 0, array.length);
		
	}
	
	/**
	 * 基数排序。
	 * 这个排序将数组中的有符号数看成是无符号数，因此，排序的结果和其他方法不同。
	 * 排序按照数的十六进制进行排序。每次排四个位。
	 * @param array
	 */
	public static void radixSort(int[] array)
	{
		if(array == null){
			return;
		}
		//show16(array);
		int[] tmp, re = null;
		int[] dest = new int[array.length];
		System.arraycopy(array, 0, dest, 0, array.length);
		for(int i = 0; i < 32; i += 4){
			randixSortHelp(dest, array, i);
			re = array;
			tmp = dest;
			dest = array;
			array = tmp;
			//show(re);
		}
		System.arraycopy(re, 0, array, 0, re.length);
	}
	
	/**
	 * 这实际上是一个counting sort。
	 * 这个counting sort依据数组中某四个位来对数组进行排序。
	 * bit指示这四个位的最低位。
	 * 结果存放在dest中。
	 * @param src, dest
	 * @param bit
	 */
	private static void randixSortHelp(int[] src, int[] dest, int bit)
	{
		if(src == null || dest == null
				|| src.length > dest.length || bit >= 32){
			return;
		}
		
		//四个位表述的数最大是15
		int[] c = new int[16];
		
		for(int i = 0; i < src.length; ++i){
			++c[getFourBit(src[i], bit)];
		}
		
		for(int i = 1; i < c.length; ++i){
			c[i] += c[i - 1];
		}
		
		for(int i = src.length - 1; i >= 0; --i){
			dest[c[getFourBit(src[i], bit)] - 1] = src[i];
			--c[getFourBit(src[i], bit)];
		}
	}
	
	/**
	 * 返回以bit位为最第位的连续四个位所表示的整数。
	 * @param n
	 * @param bit
	 * @return
	 */
	private static int getFourBit(int n, int bit){
		n >>= bit;
		n &= 0x0f;
		return n;
	}
	
	private static void show16(int[] array)
	{
		for(int i = 0; i < array.length; ++i){
			for(int j = 0; j < 32; j += 4){
				System.out.printf("%x "
						, getFourBit(array[i], j));
			}
			System.out.println();
		}
	}
	
	private static void show(int[] a)
	{
		for(int i = 0; i < a.length; ++i){
			System.out.printf("%08x ", a[i]);
		}
		System.out.printf("\n");
	}
	/**
	 * 归并排序
	 * 排序数组array， 
	 * @param array
	 * @param off
	 * @param len
	 */
	private static void mergeSortHelp(int[] src, int[] dest, 
						int off, int len){
		if(len <= 1){
			return;
		}
		
		int midlen = len >> 1;
		mergeSortHelp(dest, src, off, midlen);
		mergeSortHelp(dest, src, off + midlen, len - midlen);
		
		int left = off;
		int right = off + midlen;
		int k = off;
		while(true){
			if(left >= off + midlen || right >= off + len){
				break;
			}
			if(uless(src[left], src[right])){
				dest[k] = src[left];
				++left;
			}
			else{
				dest[k] = src[right];
				++right;
			}
			++k;
		}
		
		while(left < off + midlen){
			dest[k] = src[left];
			++left;
			++k;
		}
		
		while(right < off + len){
			dest[k] = src[right];
			++right;
			++k;
		}
	}
	/**
	 * 快速排序
	 * 排序数组array， 从off开始len长度的子数组。
	 * @param array
	 * @param off
	 * @param len
	 */
	private static void quickSortHelp(int[] array, int off, int len)
	{
		//User insertion sort
		if(len < 50){
			insertionSortHelp(array, off, len);
			return;
		}
		
		//partition
		int mid = off + (len >> 1);
		swap(array, off, mid);
		int val = array[off];
		
		int left = off, right = off + len;
		while(true){			
			do{
				++left;
			}while(left <= right && ulesseq(array[left], val));
			do{
				--right;
			}while(left <= right && ugreateq(array[right], val));
			if(left > right){
				break;
			}
			swap(array, left, right);
		}
		swap(array, off, left - 1);
		
		quickSortHelp(array, off, left - 1 - off);
		quickSortHelp(array, left, len + off - left );
	}
	
	/**
	 * 插入排序
	 * 排序数组array， 从off开始len长度的子数组。
	 * @param array
	 * @param off
	 * @param len
	 */
	private static void insertionSortHelp(int[] array, int off, int len)
	{
		int end = len + off;
		for(int i = off; i < end; ++i){
			for(int j = i; j > off; --j){
				if(ugreateq(array[j], array[j - 1])){
					break;
				}
				swap(array, j, j - 1);
			}
		}
	}
	
	/**
	 * 交换i和j位置的元素
	 * @param array
	 * @param i
	 * @param j
	 */
	private static void swap(int[] array, int i, int j){
		int tmp = array[i];
		array[i] = array[j];
		array[j] = tmp;
	}
	
	/*
	 * 将a和b看作无符号数，比较其大小
	 */

	/**
	 * a < b
	 * @param a
	 * @param b
	 * @return
	 */
	private static boolean uless(int a, int b)
	{
		int aa, bb;
		for(int i = 28; i >= 0; i -= 4){
			/*
			 * 将a和b看作是十六进制的数，aa和bb中存储的是
			 * a和b的每一位的数值。
			 * 如，a为0x3df32ad3，那么aa中存放的就分别是
			 * 3,d,f,3,2,a,d,3
			 */
			aa = (a >> i) & 0xf;
			bb = (b >> i) & 0xf;
			if(aa < bb){
				return true;
			}
			else if(aa > bb){
				return false;
			}
		}
		//a==b
		return false;
	}
	
	/**
	 * a >= b
	 * @param a
	 * @param b
	 * @return
	 */
	private static boolean ugreateq(int a, int b)
	{
		int aa, bb;
		for(int i = 28; i >= 0; i -= 4){
			aa = (a >> i) & 0xf;
			bb = (b >> i) & 0xf;
			if(aa < bb){
				return false;
			}
			else if(aa > bb){
				return true;
			}
		}
		//a==b
		return true;
	}
	
	/**
	 * a <= b
	 * @param a
	 * @param b
	 * @return
	 */
	private static boolean ulesseq(int a, int b)
	{
		int aa, bb;
		for(int i = 28; i >= 0; i -= 4){
			aa = (a >> i) & 0xf;
			bb = (b >> i) & 0xf;
			if(aa > bb){
				return false;
			}
			else if(aa < bb){
				return true;
			}
		}
		//a==b
		return true;
	}
}
