package closestPoints;

import java.util.Comparator;

/**
 * 表示点。
 * @author hcy
 *
 */
public class DPoint
{
	public double x,y;
	public int xIndex, yIndex;
	public static XCmpor xcmpor = new XCmpor();
	public static YCmpor ycmpor = new YCmpor();
	
	public DPoint()
	{
		x = y = 0.0;
		xIndex = yIndex = -1;
	}
	
	public DPoint(double x, double y)
	{
		this();
		this.x = x;
		this.y = y;
	}
	
	public String toString()
	{
		return " (" + x + "," + y + ") ";
	}
	
	public double distTo(DPoint p)
	{
		if(p == null){
			return Double.NaN;
		}
		
		return Math.sqrt((x - p.x) * (x - p.x) 
				+ (y - p.y) * (y - p.y));
	}
	
	/**
	 * 用于按x排序。
	 * @author hcy
	 *
	 */
	private static class XCmpor implements Comparator<DPoint>
	{

		@Override
		public int compare(DPoint p1, DPoint p2) 
		{
			// TODO Auto-generated method stub
			return (int)(p1.x - p2.x);
		}
		
	}
	
	/**
	 * 用于按y排序
	 * @author hcy
	 *
	 */
	private static class YCmpor implements Comparator<DPoint>
	{

		@Override
		public int compare(DPoint p1, DPoint p2) 
		{
			// TODO Auto-generated method stub
			return (int)(p1.y - p2.y);
		}
		
	}
}
