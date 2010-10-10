package sortCmp;


public class Main
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		test(10);
		int n = 10;
		for(int i = 1; i < 8; ++i){
			//test(n);
			n *= 10;
		}
	}
	
	public static void test(int n)
	{
		RandomIntegers rl = new RandomIntegers();
		int[] input = new int[n];
		rl.getIntegers(input);
		
		System.out.println("Sort (" + n + ") numbers. Begin sort...");
		
		long begin,end;
		
		rl.getIntegers(input);
		begin = System.currentTimeMillis();
		SortUtil.quickSort(input);
		end = System.currentTimeMillis();
		System.out.printf("Quicksort:\t\t%dms.\n", (end - begin));
		show(input);
		
		rl.getIntegers(input);
		begin = System.currentTimeMillis();
		SortUtil.mergeSort(input);
		end = System.currentTimeMillis();
		System.out.printf("Mergesort:\t\t%dms.\n", (end - begin));
		show(input);
		
		rl.getIntegers(input);
		begin = System.currentTimeMillis();
		SortUtil.radixSort(input);
		end = System.currentTimeMillis();
		System.out.printf("Radixsort:\t\t%dms.\n", (end - begin));
		show(input);
		
		if(n < 1000000){
			rl.getIntegers(input);
			begin = System.currentTimeMillis();
			SortUtil.insertionSort(input);
			end = System.currentTimeMillis();
			System.out.printf("Insertion sort:\t\t%dms.\n"
					, (end - begin));
			show(input);
			
		}
		System.out.println();
	}
	
	private static void show(int[] a)
	{
		for(int i = 0; i < a.length; ++i){
			System.out.printf("%x\t", a[i]);
		}
		System.out.printf("\n");
	}

}
