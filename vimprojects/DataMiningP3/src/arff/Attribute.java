/**
 * 表示一个属性及其对应的值。
 */
package arff;

public class Attribute
{
	public AttributeClass aclass; 	//属性对应的类别。
	public int val;			//属性值。对于类别属性，表示对应类别值的标号。
	
	public Attribute(AttributeClass c, int val)
	{
		this.aclass = c;
		this.val = val;
	}
	
	/*
	 * 计算两个属性之间的相异度。
	 */
	public double distance(Attribute a)
	{
		if(a == null || a.aclass != aclass){
			return -1.0;
		}
		if(aclass.type == AttributeType.CATE){
			if(a.val == val){
				return 1.0;
			}
		}
		if(aclass.type == AttributeType.NUME){
			return Math.abs(a.val - val);
		}
		return -1.0;
	}
		
	public String toString()
	{
		if(aclass.type == AttributeType.NUME){
			return ((Integer)val).toString();
		}else{
			return aclass.getCate(val);
		}
	}
}
