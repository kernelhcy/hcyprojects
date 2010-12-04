/**
 * 表示属性类别。 包括属性类型，属性的屈指范围。
 */
package arff;

import java.util.ArrayList;

public class AttributeClass
{
	public AttributeType type;
	public String name; 			//属性的名称
	
	public AttributeClass(AttributeType type, String name)
	{
		cates = new ArrayList<String>();
		this.type = type;
		this.name = name;
	}
	
	/**
	 * 增加类别。cate
	 * @parm cate : 表示类别的字符串
	 */
	public void addCate(String cate)
	{
		cates.add(cate);	
	}
	
	/**
	 * 获取类别值对应的标号。
	 */
	public int getCateIndex(String cate)
	{
		for(int i = 0; i < cates.size(); ++i){
			if(cate.compareTo(cates.get(i)) == 0){
				return i;
			}
		}
		return -1;
	}
	
	/*
	 * 返回分类值对应的名称。
	 */
	public String getCate(int index)
	{
		if(index == -1){
			return "?"; //值缺失
		}
		return cates.get(index);
	}
	
	/**
	 * 判断属性中是否包含有这个类别。
	 */
	public boolean hasCate(String cate)
	{
		//数值类型总是返回true
		if(type == AttributeType.NUME){
			return true;
		}
		return cates.contains(cate);
	}
	
	public String toString()
	{
		StringBuffer sb = new StringBuffer("@Attribute ");
		sb.append(name);
		if(type == AttributeType.NUME){
			sb.append(" numeric\n");
			return sb.toString();
		}
		
		sb.append(" {");
		for(String s : cates){
			sb.append(s);
			sb.append(',');
		}
		sb.append("}\n");
		return sb.toString();
	}
	
	private ArrayList<String> cates; 	//categorical类型对应的类别。
	
}
