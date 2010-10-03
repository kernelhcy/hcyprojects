package closestPoints;
/**
 * 保存结果。
 * @author hcy
 *
 */
public class CPResult
{
	DPoint p1, p2;
	double dist;
	
	public CPResult()
	{
		p1 = p2 = null;
		dist = Double.MAX_VALUE;
	}
}
