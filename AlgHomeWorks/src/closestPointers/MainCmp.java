package closestPointers;

public class MainCmp
{

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		MainCmp cmp = new MainCmp();
		cmp.test(10);
		cmp.test(100);
		cmp.test(1000);
		cmp.test(10000);
		cmp.test(100000);
		cmp.test(1000000);
	}
	
	/**
	 * 测试
	 * @param num
	 */
	private void test(int num)
	{
		System.out.println(num + " points:");
		points = rpg.getPoints(num);
		cpg.setPoints(points);
		start = System.currentTimeMillis();
		cpg.run(ClosestPointersGenerator.NORMAL);
		CPResult re = cpg.getResult();
		System.out.println("The result: " + re.p1 + re.p2 
				+ "dist: " + re.dist);
		end = System.currentTimeMillis();
		System.out.println("Cost time:" + (end - start) / 1000 + "s.");
	}
	
	private RandomPointsGenor rpg = new RandomPointsGenor();
	private ClosestPointersGenerator cpg = new ClosestPointersGenerator();
	private DPoint[] points;
	private double start, end;
}
