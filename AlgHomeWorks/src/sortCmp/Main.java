package sortCmp;


public class Main
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		//test(100);
		int n = 10;
		/*
		 * 由于把代码从内存提取到高速缓存需要一定的时间。因此，
		 * 在数据量较小的时候，这个时间会显著的影响程序的运行时间。
		 * 比如，在数据量为10的时候，理论上运行时间都应该在1ms以内
		 * 但是第一个运行的算法的运行时间会大于1ms。
		 */
		for(int i = 1; i < 9; ++i){
			
			test(n);
			n *= 10;
		}
		n /= 10;
		test(n);
		test(2 * n);
	}
	
	public static void test(int n)
	{
		RandomIntegers rl = new RandomIntegers();
		int[] input = new int[n];
		//int[] tmp = new int[n];
		rl.getIntegers(input);
		//System.arraycopy(input, 0, tmp, 0, input.length);
		
		System.out.println("Sort (" + n + ") numbers. Begin sort...");
		
		long begin,end;
		
		if(n <= 1000000){
			//rl.getIntegers(input);
			//System.arraycopy(tmp, 0, input, 0, input.length);
			begin = System.currentTimeMillis();
			SortUtil.insertionSort(input);
			end = System.currentTimeMillis();
			System.out.printf("Insertion sort:\t\t%dms.\n"
					, (end - begin));
			//show(input);	
		}
		
		//rl.getIntegers(input);
		rl.getIntegers(input);
		begin = System.currentTimeMillis();
		SortUtil.quickSort(input);
		end = System.currentTimeMillis();
		System.out.printf("Quicksort:\t\t%dms.\n", (end - begin));
		//show(input);
		
		if(n <= 100000000){
			//rl.getIntegers(input);
			rl.getIntegers(input);
			begin = System.currentTimeMillis();
			SortUtil.mergeSort(input);
			end = System.currentTimeMillis();
			System.out.printf("Mergesort:\t\t%dms.\n", (end - begin));
			//show(input);
			
			//rl.getIntegers(input);
			rl.getIntegers(input);
			begin = System.currentTimeMillis();
			SortUtil.radixSort(input);
			end = System.currentTimeMillis();
			System.out.printf("Radixsort:\t\t%dms.\n", (end - begin));
			//show(input);
		}
			
		System.out.println();
	}
	
	private static void show(int[] a)
	{
		for(int i = 0; i < a.length; ++i){
			System.out.printf("%08x ", a[i]);
		}
		System.out.printf("\n");
	}

}
