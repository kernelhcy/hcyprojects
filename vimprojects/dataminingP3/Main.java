import arff.*;
import kmeans.*;
public class Main
{
	public static void main(String[] arg)
	{
		String fileName = "adult_cluster.arff";
		Arff arff = ARFFParser.parse(fileName);
		System.out.printf("Attribute num : %d, Item num: %d\n"
				, arff.aclasses.size()
				, arff.items.size());
		
		KMeans km = new KMeans(arff, 2);
		km.run();
		return;	
	}
}
