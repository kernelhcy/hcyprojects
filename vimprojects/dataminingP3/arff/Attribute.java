/**
 * 表示一个属性及其对应的值。
 */
package arff;

public class Attribute
{
	public AttributeClass aclass; 	//属性对应的类别。
	public Object val;		//属性值
	
	public Attribute(AttributeClass c, Object val)
	{
		this.aclass = c;
		this.val = val;
	}
	
	public String toString()
	{
		if(aclass.type == AttributeType.NUME){
			return ((Integer)val).toString();
		}else{
			return (String)val;
		}
	}
}
