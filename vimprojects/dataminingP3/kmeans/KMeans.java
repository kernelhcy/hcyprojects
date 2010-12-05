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
		kcenters = new Item[k];
		kgroups = new ArrayList<ArrayList<Item>>();
		for(int i = 0; i < k; ++i){
			kgroups.add(new ArrayList<Item>());
		}
		size = d.items.size();
		
		E = Double.MAX_VALUE;
		numAttrs = d.items.get(1).attrs.size(); //属性的个数。
		maxMin = new int[2][numAttrs];
	}
	
	/**
	 * 进行聚类
	 */
	public void run()
	{
		System.out.printf("init...\n");
		init();
		
		System.out.printf("Select %d centers\n", k);
		randomSelectKCenters();
		
		int cnt = 1;		
		double olde;
		do{
			System.out.printf("%d:\n", cnt++);
			selectGroup();
			olde = E;
			E = calculateE();
			System.out.printf("E : %f\n", E);
			for(int i = 0; i < k; ++i){
				kcenters[i] = getMean(i);
				System.out.printf("Center %d %s\n"
						, i, kcenters[i]);	
			}
		}while(E < olde);
		
	}
	
	/*
	 * 初始化。
	 * 计算各个数值属性的最大值和最小值。
	 */
	private void init()
	{
		for(int i = 0; i < numAttrs; ++i){
			maxMin[0][i] = Integer.MIN_VALUE; 	//最大值
			maxMin[1][i] = Integer.MAX_VALUE;	//最小值
		}
	
		Attribute a;
		Item item;
		for(int i = 0; i < size; ++i){
			item = data.items.get(i);
			for(int j = 0; j < numAttrs; ++j){
				a = item.attrs.get(j);
				if(a.aclass.type == AttributeType.NUME){
					if(a.val > maxMin[0][j]){
						maxMin[0][j] = a.val;
					}
					if(a.val < maxMin[1][j]){
						maxMin[1][j] = a.val;
					}
				}
			}
			
		}
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
		Item item;
		
		for(int i = 0; i < size; ++i){
			item = data.items.get(i);
			for(int j = 0; j < k; ++j){
				tmp = itemDiff(item, kcenters[j]);
				if(tmp < nearest){
					nearest = tmp;
					nearestGI = j;
				}
			}
			kgroups.get(nearestGI).add(item);
		}

		//将中心点加入组中。
		for(int i = 0; i < k; ++i){
			kgroups.get(i).add(kcenters[i]);
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
		ArrayList<Item> g;
		
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
			kcenters[i] = data.items.get(i);
			System.out.printf("Center %d: %s\n", i
					, data.items.get(i));
		}
	}	

	/**
	 * 计算第i个数据项和第j个数据项的相异度。
	 */
	private double itemDiff(Item i1, Item i2)
	{
		if(i1 == null || i2 == null){
			return -1;
		}
		
		double dis = 0.0, qf, df;
		double up = 0.0, down = 0.0; 	//分别表示d(i,j)的分子和分母。
		
		Attribute t1, t2;

		int tmp1, tmp2, mmdiff;
		
		for(int ii = 0; ii < numAttrs; ++ii){
			t1 = i1.attrs.get(ii);
			t2 = i2.attrs.get(ii);
			if(t1.aclass != t2.aclass){
				System.err.printf("Error. Different type" 
					+ " attribute.");
				return -1.0;
			}
						
			if(t1.aclass.type == AttributeType.NUME){
				mmdiff = maxMin[0][ii] - maxMin[1][ii];
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
				down += qf;;
			}
		}
		return up / down;
	}

	/*
	 * 计算第k个簇的均值点。
	 * 对于数值类型的属性，取其均值。
	 * 对于分类类型的属性，取最多的那个值。
	 */
	private Item getMean(int k)
	{
		Item item;
		Attribute a;
		ArrayList<Item> group = kgroups.get(k);
		int gsize = group.size();
		
		int sum, cnt; 	//用于求和和计数。
		int[] indexCnt;	//记录分类属性各个值的个数。
		
		int[] attrs = new int[numAttrs];
		
		for(int i = 0; i < numAttrs; ++i){
			a = group.get(0).attrs.get(i);
			sum = 0;
			cnt = 0;
			if(a.aclass.type == AttributeType.NUME){
				for(int j = 0; j < gsize; ++j){
					item = group.get(j);
					a = item.attrs.get(i);
					sum += a.val;
					++cnt;
				}
				attrs[i] = (sum / cnt);
			}else if(a.aclass.type == AttributeType.CATE){
				indexCnt = new int[a.aclass.getCateNum()];
				for(int j = 0; j < gsize; ++j){
					item = group.get(j);
					a = item.attrs.get(i);
					if(a.val != -1){
						++indexCnt[a.val];
					}
				}
				int max = -1, maxIndex = -1;
				for(int j = 0; j < indexCnt.length; ++j){
					if(max < indexCnt[j]){
						max = indexCnt[j];
						maxIndex = j;
					}
				}
				attrs[i] = maxIndex;
			}else{
				attrs[i] = 0;
			}
		}
		
		//生成一个Item实例，表示均值点。
		Item c = new Item();
		item = data.items.get(1); 	//模板
		for(int i = 0; i < attrs.length; ++i){
			a = new Attribute(item.attrs.get(i).aclass, attrs[i]);
			c.addAttr(a);
		}
		
		return c;
	}

	private Arff data;
	private int k;				//分组个数。
	private Item[] kcenters; 		//k个中心的索引位置。
	private ArrayList<ArrayList<Item>> kgroups;//k个组。存储下标。
	private int size; 			//数据量
	private int[][] maxMin; 		//保存各个属性的最大值和最小值
	private int numAttrs;			//属性的个数。
	private double E; 			//平方误差。
}
