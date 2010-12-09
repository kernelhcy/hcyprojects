/**
 * 表示一个arff文件及其内容。
 */
package arff;

import java.util.ArrayList;
 
public class Arff
{
	public ArrayList<AttributeClass> aclasses; 	//数据的类别和取值范围
	public ArrayList<Item>  items;			//所有的数据项。
	public String relation;
	
	public Arff()
	{
		aclasses = new ArrayList<AttributeClass>();
		items = new ArrayList<Item>();
	}
	
	/**
	 * 添加一个属性类别。
	 */
	public void addAttrClass(AttributeClass c)
	{
		aclasses.add(c);
	}
	
	public void addItem(Item i)
	{
		items.add(i);
	}
	public String toString()
	{
		StringBuffer sb = new StringBuffer();
		for(AttributeClass ac : aclasses){
			sb.append(ac.toString());
		}
		for(Item i : items){
			sb.append(i.toString());
		}
		return sb.toString();
	}
}
