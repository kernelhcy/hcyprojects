/**
 * ��ʾһ����¼��
 */
package arff;

import java.util.ArrayList; 

public class Item
{
	public static long id_tem = 1; //idģ��
	
	public long id;				//��¼��id
	public ArrayList<Attribute> attrs;	//��¼��Ӧ�����Ժ�����ֵ��
	
	public Item()
	{
		++Item.id_tem;
		id = Item.id_tem;
		attrs = new ArrayList<Attribute>();
	}
	
	/**
	 * ����һ�����Ժ�ֵ��
	 */
	public void addAttr(Attribute a)
	{
		attrs.add(a);
	}
	
	/**
	 * ��ü�¼��Ӧ�����Ե�ֵ
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
