package closestPoints;

import java.util.Random;

/**
 * 生成随机点对
 * @author hcy
 *
 */
public class RandomPointsGenor
{
	
	public RandomPointsGenor(){
		rand = new Random();
	}
	
	public DPoint[] getPoints(int num)
	{
		if(num <= 0){
			return new DPoint[0];
		}
		
		x = new double[num];
		y = new double[num];
		points = new DPoint[num];
		
		for(int i = 0; i < num; ++i){
			x[i] = i;
			y[i] = i;
		}
		double tmp;
		int index;
		for(int i = 0; i < num; ++i){
			index = rand.nextInt(num - i) + i;
			tmp = x[i];
			x[i] = x[index];
			x[index] = tmp;
			
			index = rand.nextInt(num - i) + i;
			tmp = y[i];
			y[i] = y[index];
			y[index] = tmp;
			
		}
		
		for(int i = 0; i < num; ++i){
			points[i] = new DPoint();
			points[i].x = x[i];
			points[i].y = y[i];
		}
		
		//show();
		return points;
	}
	
	private void show()
	{
		if(points.length < 1000){
			for(int i = 0; i < points.length; ++i){
				if(i !=0 && i % 10 == 0){
					System.out.println();
				}
				System.out.print(points[i]);
			}
			System.out.println();
		}
	}
	
	private DPoint[] points;
	private double[] x;
	private double[] y;
	private Random rand;
}
