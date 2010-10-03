package closestPoints;

public class MainCmp
{

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		MainCmp cmp = new MainCmp();
		
		cmp.test(1000, ClosestPointersGenerator.DC);
		cmp.test(10000, ClosestPointersGenerator.DC);
		cmp.test(100000, ClosestPointersGenerator.DC);
		cmp.test(1000000, ClosestPointersGenerator.DC);
		
		cmp.test(1000, ClosestPointersGenerator.NORMAL);
		cmp.test(10000, ClosestPointersGenerator.NORMAL);
		cmp.test(100000, ClosestPointersGenerator.NORMAL);
		cmp.test(1000000, ClosestPointersGenerator.NORMAL);
		
	}
	
	/**
	 * 测试
	 * @param num
	 */
	private void test(int num, int method)
	{
		
		points = rpg.getPoints(num);
		cpg.setPoints(points);
		start = System.currentTimeMillis();
		cpg.run(method);
		CPResult re = cpg.getResult();
		
		System.out.println("###########################");
		switch (method) {
		case ClosestPointersGenerator.NORMAL:
			System.out.println("Normal:");
			break;
		case ClosestPointersGenerator.DC:
			System.out.println("Divide and Conquer:");
			break;
		default:
			break;
		}
		System.out.println(num + " points:");
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
