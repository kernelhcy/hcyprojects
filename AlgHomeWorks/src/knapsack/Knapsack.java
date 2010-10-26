package knapsack;

public class Knapsack
{
	private Knapsack() throws Exception
	{
		throw new Exception("Can NOT create an instance of Knapsack!");
	}
	
	/**
	 * 计算背包问题的最优解
	 * @param w 物品的重量数组
	 * @param v 物品的价值数组
	 * @param pc 背包的容量
	 * @return 最大的质量
	 */
	public static int run(final int[] w, final int[] v, int pc)
	{
		if(w == null || v == null 
			|| w.length == 0 || v.length == 0
			|| w.length != v.length
			|| pc <= 0){
			return -1;
		}
		
		/*
		 * 保存数据，用于构造最优解。
		 */
		Knapsack.pc = pc;
		Knapsack.len = w.length;
		Knapsack.v = new int[v.length];
		Knapsack.w = new int[w.length];
		System.arraycopy(w, 0, Knapsack.w, 0, w.length);
		System.arraycopy(v, 0, Knapsack.v, 0, v.length);
		
		int[] p = new int[pc + 1];
		pre = new int[w.length + 1][pc + 1];
			
		/*
		 * p[i][j]表示前i个物品放到容量为j的背包中的最大价值。
		 * 分两种情况：第i个物品放包中和不放包中。
		 * p[i][j] = max(p[i - 1][j], p[i - 1][j - w[i]] + v[i])
		 * 上面的递推式标明，每次计算只使用上行的数值，因此可以将p压缩成
		 * 一个一维数组。从而减少空间。
		 * p[j] = max(p[j], p[j - w[i]] + v[i])
		 */
		//初始化只有一个物品的情况。
		for(int i = w[0];i <= pc; ++i){
			p[i] = v[0];
			pre[1][i] = 1;
		}
		for(int i = 2; i <= w.length; ++i){
			for(int j = pc; j > 0; --j){
				if(j >= w[i - 1] 
				     && p[j - w[i - 1]] + v[i - 1]
				                   > p[j]){
					p[j] = p[j - w[i - 1]] 
					            + v[i - 1];
					pre[i][j] = 1;
				}else{
					pre[i][j] = 0;
				}
			}
		}
		
		return p[pc];
	}
	
	/**
	 * 构造最优解。
	 * 返回的数组中，如果re[i] == 1，那么第i+1个物品被选中。
	 * @return
	 */
	public static int[] getResult()
	{
		int[] re = new int[len];
		for(int i = len, j = pc; i > 0; --i){
			if(pre[i][j] == 1){
				re[i - 1] = 1;
				j -= w[i - 1];
			}else{
				re[i - 1] = 0;
			}
		}
		return re;
	}
	
	/*
	 * 用于构造最优解
	 * pre[i][j]保存前i物品供选择且背包容量为j是，第i个物品是否放入包中。
	 */
	private static int[][] pre;
	
	//物品重量和价值。
	private static int[] w;
	private static int[] v;
	//背包容量及物品个数。
	private static int pc;
	private static int len;
}
