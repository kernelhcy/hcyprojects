package kmeans;

import arff.*;
import java.util.Random;
import java.util.ArrayList;
import java.io.*;
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
		numAttrs = d.items.get(0).attrs.size(); //属性的个数。
		maxMin = new int[2][numAttrs];
	}
	
	/**
	 * 进行聚类
	 */
	public void run() throws IOException
	{
		ByteArrayOutputStream baos = new ByteArrayOutputStream(500);
		PrintStream ps = new PrintStream(baos, true);
		PrintStream oldps = System.out;
		System.setOut(ps);
	
		System.out.printf("=== Run information ===\n\n");
		System.out.printf("Scheme:\t\tKMeans -k %d\n", k);
		System.out.printf("Relation:\t%s\n", data.relation);
		System.out.printf("Instances:\t%d\n", data.items.size());
		System.out.printf("Attributes:\t%d\n", numAttrs);
		AttributeClass ac;
		for(int i = 0; i < numAttrs; ++i){
			ac = data.items.get(0).attrs.get(i).aclass;
			//%-30s靠右对其，最少写30个字符。
			System.out.printf("\t\t%-30s%s\n"
					, ac.name, ac.type.getName());
		}
		
		init();
		
		randomSelectKCenters();
		
		int cnt = 1;		
		double olde;
		ArrayList<ArrayList<Item>> oldgroups;
		do{
			++cnt;
			oldgroups = kgroups;
			kgroups = new ArrayList<ArrayList<Item>>();
			for(int i = 0; i < k; ++i){
				kgroups.add(new ArrayList<Item>());
			}
			
			selectGroup();
			olde = E;
			E = calculateE();
			for(int i = 0; i < k; ++i){
				kcenters[i] = getMean(i);
			}
		}while(E < olde);
		kgroups = oldgroups;
		//remove the last center.
		for(ArrayList<Item> g : kgroups){
			g.remove(g.size() - 1);
		}


		System.out.printf("KMeans\n=====\n\n" 
				+ "Number of iterations: %d\n\n"
				, --cnt);
		System.out.printf("Cluster centroids:\n\n");
		for(int i = 0; i < k; ++i){
			System.out.printf("Cluster %d:\n"
					+ "\tInstances %d\n"
					+ "\tMean: %s\n"
					, i, kgroups.get(i).size()
					, kcenters[i]);
		}
		System.out.printf("\nE: \n\t%f\n", E);
		System.out.printf("\nClustered Instances:\n\n");
		for(int i = 0, s; i < k; ++i){
			s = kgroups.get(i).size();
			System.out.printf("\t%d\t%d\t(%.0f%%)\n"
					,i, s
					, ((float)s/(float)size * 100));
		}
		
		showGroupCnt(data.items.get(0).attrs.size() - 1);
		
		System.out.printf("\nEntropy:\n\n\t%s\n"
			, entropy(data.items.get(0).attrs.size() - 1));

		result = new String(baos.toByteArray());
		baos.close();
		ps.close();
		
		System.setOut(oldps);
		System.out.println(result);		
	}
	
	public String getResult()
	{
		return result;
	}

	private void showGroupCnt(int attrIndex)
	{
		int cateNum = data.aclasses.get(attrIndex).getCateNum();
		int[][] gcnt = new int[k][cateNum];
		int gindex = 0;
		Attribute a;
		for(ArrayList<Item> g : kgroups){
			for(Item item : g){
				a = item.attrs.get(attrIndex);
				for(int i = 0; i < cateNum; ++i){
					if(a.val == i){
						++gcnt[gindex][i];
					}
				}
			}
			++gindex;
		}
		System.out.printf("\n\nCluster Result according to %s\n"
				, data.aclasses.get(attrIndex));
		System.out.printf("\tCluster\t");
		for(int i = 0; i < cateNum; ++i){
			System.out.printf("\t%s"
				, data.aclasses.get(attrIndex).getCate(i));
		}
		System.out.println();
		for(int i = 0; i < k; ++i){
			System.out.printf("\t%d\t", i);
			for(int j = 0; j < cateNum; ++j){
				System.out.printf("\t%d", gcnt[i][j]);
			}
			System.out.println();
		}
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
			nearest = Double.MAX_VALUE;
			nearestGI = 0;
		}

		//将中心点加入组中。
		for(int i = 0; i < k; ++i){
			kgroups.get(i).add(kcenters[i]);
		}

	}
	
	/*
	 * 计算平方误差。
	 */
	private double calculateE()
	{
		double e = 0.0, tmp;
		ArrayList<Item> g;
		int s;
		for(int i = 0; i < k; ++i){
			g = kgroups.get(i);
			s = g.size();
			for(int j = 0; j < s; ++j){
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
		Random r = new Random(100); //设置seed。
		int rand, index;
		for(int i = 0; i < k; ++i){
			rand = r.nextInt(per);
			index = per * i + rand;
			kcenters[i] = data.items.get(index);
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
				down += qf;
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

	private double entropy(int attrIndex)
	{
		double en = 0.0, enTotal = 0.0;
		double pr;
		if(data.aclasses.get(attrIndex).type != AttributeType.CATE){
			System.err.println("Please select an cate attribute.");
			return 0.0;
		}
		int cateSize = data.aclasses.get(attrIndex).getCateNum();
		for(ArrayList<Item> g : kgroups){
			en = 0.0;
			for(int i = 0; i < cateSize; ++i){
				pr = proportion(attrIndex, i, g);
				en += (pr * (Math.log(pr) / Math.log(2)));
			}
			en = -en;
			enTotal += ((double)g.size() / (double)data.items.size()
					* en );
		}
		return enTotal;
	}

	private double proportion(int attrIndex, int classVal
				, ArrayList<Item> g)
	{
		double pr = 0.0;
		int cjSize = g.size();
		int ciCnt = 0;
		Attribute a;
		for(Item item : g){
			a = item.attrs.get(attrIndex);
			if(a.val == classVal){
				++ciCnt;
			}
		}
		pr = (double)ciCnt / (double)cjSize;
		return pr;
	}

	private Arff data;
	private int k;				//分组个数。
	private Item[] kcenters; 		//k个中心的索引位置。
	private ArrayList<ArrayList<Item>> kgroups;//k个组。存储下标。
	private int size; 			//数据量
	private int[][] maxMin; 		//保存各个属性的最大值和最小值
	private int numAttrs;			//属性的个数。
	private double E; 			//平方误差。
	private String result;			//输出的结果。
	private int[][] groupCnt;
}
