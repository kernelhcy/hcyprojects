package kmeans;

import arff.*;
import java.util.Random;
import java.util.ArrayList;

/**
 * KMeans 算法实现
 */
public class KMeans
{
	public KMeans(Arff d, int k)
	{
		data = d;
		this.k = k;
		kcenters = new int[k];
		kgroups = new ArrayList<ArrayList<Integer>>();
		for(int i = 0; i < k; ++i){
			kgroups.add(new ArrayList<Integer>());
		}
		size = d.items.size();
		diff = new double[size][size];
		for(int i = 0; i < size; ++i){
			for(int j = 0; j < size; ++j){
				diff[i][j] = -1.0;
			}
		}
		E = Double.MAX_VALUE;
	}
	
	/**
	 * 进行聚类
	 */
	public void run()
	{
		System.out.printf("init : Select %d centers\n", k);
		randomSelectKCenters();
		
		selectGroup();
		
		double e = calculateE();
		System.out.printf("E : %f\n", e);
	}
	
	/**
	 * 数据项选择距离中心点最近的组，加入。
	 */
	private void selectGroup()
	{
		//清除组中的数据。
		for(int i = 0; i < k; ++i){
			kgroups.get(i).clear();
		}
		
		int nearestGI = 0; 	//最近的组的索引。
		double nearest = Double.MAX_VALUE;
		double tmp;
		
		for(int i = 0; i < size; ++i){
			for(int j = 0; j < k; ++j){
				tmp = itemDiff(i, kcenters[j]);
				if(tmp < nearest){
					nearest = tmp;
					nearestGI = j;
				}
			}
			kgroups.get(nearestGI).add(i);
		}
		
		System.out.printf("Grouping done!\n");
		for(int i = 0; i < k; ++i){
			System.out.printf("Group %d has %d items\n"
					,i ,kgroups.get(i).size());
		}
	}
	
	/*
	 * 计算平方误差。
	 */
	private double calculateE()
	{
		double e = 0.0, tmp;
		int size;
		ArrayList<Integer> g;
		
		for(int i = 0; i < k; ++i){
			g = kgroups.get(i);
			size = g.size();
			for(int j = 0; j < size; ++j){
				tmp = itemDiff(kcenters[i], g.get(j));
				e += (tmp * tmp);
			}
		}
		return e;
	}
	
	/**
	 * 随机的选择k个初始中心点。
	 * 将所有的数据分成k组，从每组中随机的选择一个。
	 */
	private void randomSelectKCenters()
	{
		int per = size / k;
		Random r = new Random();
		int rand, index;
		
		for(int i = 0; i < k; ++i){
			rand = r.nextInt(per);
			index = per * i + rand;
			kcenters[i] = index;
			System.out.printf("Center %d: %s\n", index
					, data.items.get(index));
		}
	}	

	/**
	 * 计算第i个数据项和第j个数据项的相异度。
	 */
	private double itemDiff(int i, int j)
	{
		if(i < 0 || j < 0){
			return -1;
		}
		
		//查看是否已经计算过了。
		if(diff[i][j] + 1 > 0.000001){
			return diff[i][j];
		}
		
		double dis = 0.0, qf, df;
		double up = 0.0, down = 0.0; 	//分别表示d(i,j)的分子和分母。
		int[] maxMin = new int[2];
		
		Attribute t1, t2;
		Item i1, i2;
		i1 = data.items.get(i);
		i2 = data.items.get(j);
		int size = i1.attrs.size();
		
		int tmp1, tmp2, mmdiff;
		
		for(int ii = 0; ii < size; ++ii){
			t1 = i1.attrs.get(ii);
			t2 = i2.attrs.get(ii);
			if(t1.aclass != t2.aclass){
				System.err.printf("Error. Different type" 
					+ " attribute.");
				return -1.0;
			}
			
			//System.out.printf("Attr %d, type %s, name %s\n", ii
			//			, t1.aclass.type
			//			, t1.aclass.name);
						
			if(t1.aclass.type == AttributeType.NUME){
				mmdiff = getMaxAndMinDiff(ii);
				tmp1 = ((Integer)t1.val).intValue();
				tmp2 = ((Integer)t2.val).intValue();
				if(tmp1 == 0 && tmp2 == 0){
					qf = 0.0;
				}else{
					qf = 1.0;
				}
				df = (double)Math.abs(tmp1 - tmp2) 
						/ (double)mmdiff;
				up += ((double)qf * df);
				down += qf;
				//System.out.printf("\tMMDiff %d,val1 %d,val2 %d"
				//	+ ", qf %f, df %f,  up %f, down %f\n"
				//	, mmdiff, tmp1, tmp2, qf, df, up, down);
			}else if(t1.aclass.type == AttributeType.CATE){
				qf = 1.0;
				if(t1.equals(t2)){
					df = 0.0;
				}else{
					df = 1.0;
				}
				
				//属性值缺失。
				if(t1.val == -1 || t2.val == -1){
					qf = 0.0;
				}
				
				up += (qf * df);
				down += qf;
				//System.out.printf("\tval1 %s, val2 %s, df %f"
				//		+ ", qf %f, up %f, down %f\n"
				//		, t1.val, t2.val, df, qf, up
				//		, down);
			}
		}
		diff[i][j] = up /down;
		return up / down;
	}

	
	/*
	 * 计算第i个属性的最大和最小值之差
	 */
	private int getMaxAndMinDiff(int i)
	{
		if(i < 0){
			return -1;
		}
		int size = data.items.size();
		int max = Integer.MIN_VALUE;
		int min = Integer.MAX_VALUE;
		int tmp;
		for(int ii = 0; ii < size; ++ii){
			tmp = ((Integer)data.items.get(ii)
					.attrs.get(i).val).intValue();
			if(tmp > max){
				max = tmp;
			}
			if(tmp < min){
				min = tmp;
			}
		}
		
		return max - min;
	}
	

	private Arff data;
	private int k;				//分组个数。
	private int[] kcenters; 		//k个中心的索引位置。
	private ArrayList<ArrayList<Integer>> kgroups;//k个组。存储下标。
	private int size; 			//数据量
	private double[][] diff; 		//相异度矩阵。
	private double E; 			//平方误差。
}
