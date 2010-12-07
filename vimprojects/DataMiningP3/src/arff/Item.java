/**
 * 表示一条记录。
 */
package arff;

import java.util.ArrayList; 

public class Item
{
	public static long id_tem = 1; //id模板
	
	public long id;				//记录的id
	public ArrayList<Attribute> attrs;	//记录对应的属性和属性值。
	
	public Item()
	{
		++Item.id_tem;
		id = Item.id_tem;
		attrs = new ArrayList<Attribute>();
	}
	
	/**
	 * 增加一个属性和值。
	 */
	public void addAttr(Attribute a)
	{
		attrs.add(a);
	}
	
	/**
	 * 获得记录对应的属性的值
	 */
	public Attribute getAttr(AttributeClass c)
	{
		for(Attribute a : attrs){
			if(a.aclass.type == c.type){
				return a;
			}
		}
		
		return null;
	}
	
	public String toString()
	{
		StringBuffer sb = new StringBuffer();
		for(Attribute a : attrs){
			sb.append(a.toString());
			sb.append(",");
		}
		sb.append('\n');
		return sb.toString();
	}
}
