/**
 * ��ʾһ��arff�ļ��������ݡ�
 */
package arff;

import java.util.ArrayList;
 
public class Arff
{
	public ArrayList<AttributeClass> aclasses; 	//���ݵ�����ȡֵ��Χ
	public ArrayList<Item>  items;			//���е������
	public String relation;
	
	public Arff()
	{
		aclasses = new ArrayList<AttributeClass>();
		items = new ArrayList<Item>();
	}
	
	/**
	 * ���һ���������
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
