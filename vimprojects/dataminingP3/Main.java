import arff.*;

public class Main
{
	public static void main(String[] arg)
	{
		String fileName = "/home/hcy/Desktop/homeworks/DataMining/"
				+"project3/data/adult_cluster.arff";
		Arff arff = ARFFParser.parse(fileName);
		System.out.printf("Attribute num : %d, Item num: %d\n"
				, arff.aclasses.size()
				, arff.items.size());
		System.out.println(arff);
		return;	
	}
}
