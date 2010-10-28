package problem6;

/**
 * 错排。
 * 并且相邻的两个数也不连续。
 * 使用DFS遍历。结果数量较大，结果的数量在O(n!)种。不适合计算较大规模的数。
 * @author hcy
 *
 */
public class Main6
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		getResult(5);
		getResult(6);
		getResult(7);
		//getResult(8);
		//getResult(9);
		//getResult(10);
	}
	
	private static void getResult(int n)
	{
		if(n<=0){
			return;
		}
		Main6.n = n;
		pack = new int[n + 1];
		isUsed = new boolean[n + 1];
		for(int i = 0; i <= n; ++i){
			pack[i] = -1;
			isUsed[i] = false;
		}
		
		getResultHelp(1);
	}
	
	private static void getResultHelp(int m){
		if(m > n){
			++cnt;
			show();
			return;
		}
		for(int i = 1; i <= n; ++i){
			/*
			 * 不和篮子的号相同。
			 * 此球未放入篮子中。
			 * 和其前一个球不相邻。
			 */
			if(i != m && !isUsed[i]
				&& i != pack[m - 1] + 1
				&& i != pack[m - 1] - 1){
				pack[m] = i;
				isUsed[i] = true;
				getResultHelp(m + 1);
				isUsed[i] = false;
			}
		}
	}
	
	private static void show(){
		System.out.println("Case: " + cnt);
		for(int i = 1; i <=n; ++i){
			System.out.print(pack[i] + " ");
		}
		System.out.println();
	}
	
	private static int cnt = 0;
	private static int n = 0;
	private static int[] pack;
	private static boolean[] isUsed;

}
