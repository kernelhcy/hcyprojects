package closestPoints;

import java.util.Arrays;

public class ClosestPointersGenerator
{
	public static final int NORMAL = 1; 	//普通遍历
	public static final int DC = 2;	//分治
	
	public ClosestPointersGenerator(DPoint[] ps)
	{
		this.setPoints(ps);
		result = null;
	}
	
	public ClosestPointersGenerator()
	{
		result = null;
	}
	
	/**
	 * 设置点对。
	 * @param ps
	 */
	public void setPoints(DPoint[] ps)
	{
		int len = ps.length;
		points = ps;
		xSorted = new DPoint[len];
		ySorted = new DPoint[len];
		zPoints = new DPoint[len];
		for(int i = 0; i < len ; ++i){
			xSorted[i] = points[i];
			ySorted[i] = points[i];
		}
	}
	/**
	 * 获得结果。
	 * @param method
	 * @return
	 */
	public int run(int method)
	{
		switch(method){
		case NORMAL:
			this.result = getClosestPointsN(0, points.length - 1);
			break;
		case DC:
			Arrays.sort(xSorted, DPoint.xcmpor);
			Arrays.sort(ySorted, DPoint.ycmpor);
			for(int i = 0; i < xSorted.length; ++i){
				xSorted[i].xIndex = i;
				ySorted[i].yIndex = i;
			}
			
//			this.showSorted();
			this.result = getClosestPointsDC(xSorted, ySorted
					, zPoints, 0, xSorted.length - 1);
			break;
		default:
			return -1;
		}		
		return 1;
	}
	
	/**
	 * 计算[start, end]范围内的最近点对。
	 * 使用分治法。
	 * @param start
	 * @param end
	 * @return
	 */
	private CPResult getClosestPointsDC(DPoint[] x, DPoint[] y, DPoint[] z
			,int start, int end)
	{
//		System.out.println("Range:" + start + "~" + end);
		CPResult re ;
		
		//只有两个点
		if(start +1 == end){
			re = new CPResult();
			re.p1 = x[start];
			re.p2 = x[end];
			re.dist = re.p1.distTo(re.p2);
			return re;
		}
		else if(start + 2 == end){ //只有三个点。
			re = new CPResult();
			double d1,d2,d3;
			
			d1 = x[start].distTo(x[start + 1]);
			d2 = x[start].distTo(x[end]);
			d3 = x[start + 1].distTo(x[end]);
			
			if(d1 <= d2 && d1 <= d3){
				re.p1 = x[start];
				re.p2 = x[start + 1];
				re.dist = d1;
			}
			else if(d2 <= d1 && d2 <= d3){
				re.p1 = x[start];
				re.p2 = x[end];
				re.dist = d2;
			}
			else if(d3 <= d1 && d3 <= d2){
				re.p1 = x[start + 1];
				re.p2 = x[end];
				re.dist = d3;
			}
			
			return re;
		}
		
		int mid = (start + end) / 2;
		
		//重组对y有序的数组。
		int low = start;
		int up = mid + 1;
		for(int i = start; i <= end; ++i){
			if(y[i].xIndex > mid){
				z[up++] = y[i];
			}else{
				z[low++] = y[i];
			}
		}
		for(int i = start; i <= end; ++i){
			y[i] = z[i];
		}
		
		CPResult re1 = getClosestPointsDC(x, z, y, start, mid);
		CPResult re2 = getClosestPointsDC(x, z, y, mid + 1, end);
		
//		System.out.println("Range:" + start + "~" + mid 
//				+ " dist:" + re1.dist);
//		System.out.println("Range:" + (mid + 1) + "~" + end 
//				+ " dist:" + re2.dist);
		
		if(re1.dist < re2.dist){
			re = re1;
		}
		else{
			re = re2;
		}
		
		//回复y排序数组。
		this.merge(z, y, start, mid, end);
		
		int k = start;
		for(int i = start; i <= end; ++i){
			if(Math.abs(x[mid].x - y[i].x)
					< re.dist){
				z[k++] = y[i];
			}
		}
		
		double tmpDist;
		for(int i = start; i < k; ++i){
			for(int j = i + 1; j < k 
				&& z[j].y - z[i].y < re.dist; ++j){
				tmpDist = z[i].distTo(z[j]);
				if(tmpDist < re.dist){
					re.dist = tmpDist;
					re.p1 = z[i];
					re.p2 = z[j];
				}
			}
		}
//		System.out.println("Range:" + start + "~" + end 
//				+ " dist:" + re.dist + re.p1 + re.p2);
		return re;
	}
	
	/**
	 * 使用遍历
	 * @param start
	 * @param end
	 * @return
	 */
	private CPResult getClosestPointsN(int start, int end)
	{
		CPResult re = new CPResult();
		double tmpDist;
		for(int i = start; i <= end; ++i){
			for(int j = i + 1; j <= end; ++j){
				tmpDist = points[i].distTo(points[j]);
				if (tmpDist < re.dist){
					re.dist = tmpDist;
					re.p1 = points[i];
					re.p2 = points[j];
				}
			}
		}
		return re;
	}
	
	public CPResult getResult()
	{
		return result;
	}
	
	public void showSorted()
	{
		System.out.println("Sorted by x:");
		for (int i = 0; i < xSorted.length; ++i){
			System.out.print(xSorted[i]);
		}
		System.out.println();
		System.out.println("Sorted by y:");
		for (int i = 0; i < ySorted.length; ++i){
			System.out.print(ySorted[i]);
		}
		System.out.println();
	}
	
	/**
	 * merge zPoints[start~mid]和zPoints[mid + 1 ~ end]
	 * @param start
	 * @param mid
	 * @param end
	 */
	private void merge(DPoint[] x, DPoint[] y, int start, int mid, int end)
	{
		int k = start;
		int i = start, j = mid + 1;
		while(i <= mid && j <= end){
			if(x[i].y < x[j].y){
				y[k] = x[i];
				++i;
			}else{
				y[k] = x[j];
				++j;
			}
			++k;
		}
		
		for(; i <= mid; ++i){
			y[k] = x[i];
			++k;
		}
		
		for(; j <= end; ++j){
			y[k] = x[j];
			++k;
		}
		return;
	}
	
	private DPoint[] points;
	private DPoint[] xSorted;
	private DPoint[] ySorted;
	private DPoint[] zPoints;
	private CPResult result;
}
